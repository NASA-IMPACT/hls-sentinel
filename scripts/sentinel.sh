#!/bin/bash
# shellcheck disable=SC2153
# shellcheck disable=SC1091
# Exit on any error
set -o errexit

jobid="$AWS_BATCH_JOB_ID"
granulelist="$GRANULE_LIST"
bucket="$OUTPUT_BUCKET"
# shellcheck disable=SC2034
inputbucket="$INPUT_BUCKET"
workingdir="/var/scratch/${jobid}"
vidir="${workingdir}/vi"
bucket_role_arn="$GCC_ROLE_ARN"
debug_bucket="$DEBUG_BUCKET"
replace_existing="$REPLACE_EXISTING"
gibs_bucket="$GIBS_OUTPUT_BUCKET"

# Remove tmp files on exit
# shellcheck disable=SC2064
trap "rm -rf $workingdir; exit" INT TERM EXIT

# Create workingdir
mkdir -p "$workingdir"
# The derive_s2nbar C code infers values from the input file name so this
# formatting is necessary.  This implicit name requirement is not documented
# anywhere!

# The required format for nbar is read in sections from left to right
# HLS.S30.${tileid}.${year}${doy}.${obs}.nbar.${HLSVER}.hdr
set_output_names () {
  # Use the base SAFE name without the unique id for the output file name.
  IFS='_'
  read -ra granulecomponents <<< "$1"
  # Include twin in bucket key for s3 when argument is included.
  # this is necessary for LPDAAC's ingestion timing.
  twinkey=""
  if [ -n "$2" ]; then
    twinkey="/twin"
  fi
  date=${granulecomponents[2]:0:15}
  year=${date:0:4}
  month=${date:4:2}
  day=${date:6:2}
  hms=${date:8:7}

  hlsversion="v2.0"
  day_of_year=$(get_doy "${year}" "${month}" "${day}")
  outputname="HLS.S30.${granulecomponents[5]}.${year}${day_of_year}${hms}.${hlsversion}"
  vi_outputname="HLS-VI.S30.${granulecomponents[5]}.${year}${day_of_year}${hms}.${hlsversion}"
  output_hdf="${workingdir}/${outputname}.hdf"
  nbar_name="HLS.S30.${granulecomponents[5]}.${year}${day_of_year}.${hms}.${hlsversion}"
  nbar_input="${workingdir}/${nbar_name}.hdf"
  nbar_hdr="${nbar_input}.hdr"
  output_thumbnail="${workingdir}/${outputname}.jpg"
  output_metadata="${workingdir}/${outputname}.cmr.xml"
  output_stac_metadata="${workingdir}/${outputname}_stac.json"
  bucket_key="s3://${bucket}/S30/data/${year}${day_of_year}/${outputname}${twinkey}"
  vi_bucket_key="s3://${bucket}/S30_VI/data/${year}${day_of_year}/${vi_outputname}${twinkey}"
  gibs_dir="${workingdir}/gibs"
  gibs_bucket_key="s3://${gibs_bucket}/S30/data/${year}${day_of_year}"
  # We also need to obtain the sensor for the Bandpass parameters file
  sensor="${granulecomponents[0]:0:3}"
  angleoutputfinal="${workingdir}/${outputname}.ANGLE.hdf"
}

exit_if_exists () {
  if [ -n "$replace_existing" ]; then
    # Check if output folder key exists
    exists=$(aws s3 ls "${bucket_key}/" | wc -l)
    if [ ! "$exists" = 0 ]; then
      echo "Output product already exists.  Not replacing"
      exit 4
    fi
  fi
}

echo "Start processing granules"
# Create array from granulelist
IFS=','
read -r -a granules <<< "$granulelist"
# Consolidate twin granules if necessary.
if [ "${#granules[@]}" = 2 ]; then
  # Use the base SAFE name without the unique id for the output file name.
  set_output_names "${granules[0]}" twin
  # Process each granule in granulelist and build the consolidatelist
  consolidatelist=""
  consolidate_angle_list=""
  for granule in "${granules[@]}"; do
    granuledir="${workingdir}/${granule}"
    angleoutput="${granuledir}/angle.hdf"
    granuleoutput="${granuledir}/sr.hdf"
    source sentinel_granule.sh
    # Build list of outputs and angleoutputs to consolidate
    if [ "${#consolidatelist}" = 0 ]; then
      consolidatelist="${granuleoutput}"
      consolidate_angle_list="${angleoutput}"
    else
      consolidatelist="${consolidatelist} ${granuleoutput}"
      consolidate_angle_list="${consolidate_angle_list} ${angleoutput}"
    fi
  done
  echo "Running consolidate on ${consolidatelist}"
  consolidate_output="${workingdir}/consolidate.hdf"
  consolidate_angle_output="${workingdir}/consolidate_angle.hdf"
  consolidate_command="consolidate ${consolidatelist} ${consolidate_output}"
  consolidate_angle_command="consolidate_s2ang ${consolidate_angle_list} ${consolidate_angle_output}"
  eval "$consolidate_command"
  eval "$consolidate_angle_command"
  # Use the consolidate output as loop process output for next stage.
  angleoutput="$consolidate_angle_output"
  granuleoutput="$consolidate_output"
else
  # If it is a single granule, just use granule output without condolidation
  granule="$granulelist"
  set_output_names "$granule"
  exit_if_exists

  granuledir="${workingdir}/${granule}"
  angleoutput="${granuledir}/angle.hdf"
  granuleoutput="${granuledir}/sr.hdf"

  source sentinel_granule.sh
fi

# Resample to 30m
echo "Running create_s2at30m"
resample30m="${workingdir}/resample30m.hdf"
resample30m_hdr="${resample30m}.hdr"
create_s2at30m "$granuleoutput" "$resample30m"

# Unlike all the other C libs, derive_s2nbar and L8like modify the input file
# Move the resample output to nbar naming.
# Maintain intermediate 30m version in debug mode.
if [ -z "$debug_bucket" ]; then
  mv "$resample30m" "$nbar_input"
  mv "$resample30m_hdr" "$nbar_hdr"
else
  cp "$resample30m" "$nbar_input"
  cp "$resample30m_hdr" "$nbar_hdr"
fi

# Nbar
echo "Running derive_s2nbar"
cfactor="${workingdir}/cfactor.hdf"
derive_s2nbar "$nbar_input" "$angleoutput" "$cfactor"

nbarIntermediate="${workingdir}/nbarIntermediate.hdf"
nbarIntermediate_hdr="${nbarIntermediate}.hdr"
# Maintain intermediate nbar version in debug mode.
if [ "$debug_bucket" ]; then
  cp "$nbar_input" "$nbarIntermediate"
  cp "$nbar_hdr" "$nbarIntermediate_hdr"
fi

# Bandpass
echo "Running L8like"
parameter="/usr/local/bandpass_parameter.${sensor}.txt"
L8like "$parameter" "$nbar_input"

mv "$nbar_input" "$output_hdf"
mv "${nbar_input}.hdr" "${output_hdf}.hdr"

# Convert to COGs
echo "Converting to COGs"
hdf_to_cog "$output_hdf" --output-dir "$workingdir" --product S30

mv "$angleoutput" "$angleoutputfinal"
hdf_to_cog "$angleoutputfinal" --output-dir "$workingdir" --product S30_ANGLES

# Create thumbnail
echo "Creating thumbnail"
create_thumbnail -i "$workingdir" -o "$output_thumbnail" -s S30

# Create metadata
echo "Creating metadata"
create_metadata "$output_hdf" --save "$output_metadata"

# Create STAC metadata
cmr_to_stac_item "$output_metadata" "$output_stac_metadata" \
  data.lpdaac.earthdatacloud.nasa.gov 020

# Generate manifest
echo "Generating manifest"
manifest_name="${outputname}.json"
manifest="${workingdir}/${manifest_name}"
create_manifest "$workingdir" "$manifest" "$bucket_key" "HLSS30" \
  "$outputname" "$jobid" false

# Copy output to S3.
mkdir -p ~/.aws
echo "[profile gccprofile]" > ~/.aws/config
echo "region=us-east-1" >> ~/.aws/config
echo "output=text" >> ~/.aws/config

echo "[gccprofile]" > ~/.aws/credentials
echo "role_arn = ${bucket_role_arn}" >> ~/.aws/credentials
echo "credential_source = Ec2InstanceMetadata" >> ~/.aws/credentials

if [ -z "$debug_bucket" ]; then
  aws s3 cp "$workingdir" "$bucket_key" --exclude "*" --include "*.tif" \
    --include "*.xml" --include "*.jpg" --include "*_stac.json" \
    --exclude "*fmask.bin.aux.xml" --profile gccprofile --recursive

  # Copy manifest to S3 to signal completion.
  aws s3 cp "$manifest" "${bucket_key}/${manifest_name}" --profile gccprofile
else
  # Create
  # Convert intermediate hdf to COGs
  hdf_to_cog "$resample30m" --output-dir "$workingdir" --product S30 --debug-mode
  hdf_to_cog "$nbarIntermediate" --output-dir "$workingdir" --product S30 --debug-mode

  # Copy all intermediate files to debug bucket.
  echo "Copy files to debug bucket"
  debug_bucket_key=s3://${debug_bucket}/${outputname}
  aws s3 cp "$workingdir" "$debug_bucket_key" --recursive --profile gccprofile
fi

# Generate GIBS browse subtiles
echo "Generating GIBS browse subtiles"
mkdir -p "$gibs_dir"
granule_to_gibs "$workingdir" "$gibs_dir" "$outputname"
for gibs_id_dir in "$gibs_dir"/* ; do
    if [ -d "$gibs_id_dir" ]; then
      gibsid=$(basename "$gibs_id_dir")
      echo "Processing gibs id ${gibsid}"
      # shellcheck disable=SC2206
      xmlfiles=(${gibs_id_dir}/*.xml)
      xml="${xmlfiles[0]}"
      subtile_basename=$(basename "$xml" .xml)
      subtile_manifest_name="${subtile_basename}.json"
      subtile_manifest="${gibs_id_dir}/${subtile_manifest_name}"
      gibs_id_bucket_key="$gibs_bucket_key/${gibsid}"
      echo "Gibs id bucket key is ${gibs_id_bucket_key}"

      create_manifest "$gibs_id_dir" "$subtile_manifest" \
        "$gibs_id_bucket_key" "HLSS30" "$subtile_basename" "$jobid" true

      # Copy GIBS tile package to S3.
      if [ -z "$debug_bucket" ]; then
        aws s3 cp "$gibs_id_dir" "$gibs_id_bucket_key" --exclude "*"  \
          --include "*.tif" --include "*.xml" --profile gccprofile \
          --recursive --quiet

        # Copy manifest to S3 to signal completion.
        aws s3 cp "$subtile_manifest" \
          "${gibs_id_bucket_key}/${subtile_manifest_name}" \
          --profile gccprofile
      else
        # Copy all intermediate files to debug bucket.
        echo "Copy files to debug bucket"
        debug_bucket_key=s3://${debug_bucket}/${outputname}
        aws s3 cp "$gibs_id_dir" "$debug_bucket_key" --recursive --quiet \
        --profile gccprofile
      fi
    fi
done
echo "All GIBS tiles created"

# Generate VI files
echo "Generating VI files"
vi_generate_indices -i "$workingdir" -o "$vidir" -s "$outputname"
vi_generate_metadata -i "$workingdir" -o "$vidir"
vi_generate_stac_items --cmr_xml "$vidir/${vi_outputname}.cmr.xml" --endpoint data.lpdaac.earthdatacloud.nasa.gov --version 020 --out_json "$vidir/${vi_outputname}_stac.json"

echo "Generating VI manifest"
vi_manifest_name="${vi_outputname}.json"
vi_manifest="${vidir}/${vi_manifest_name}"
create_manifest "$vidir" "$vi_manifest" "$vi_bucket_key" "HLSS30_VI" \
  "$vi_outputname" "$jobid" false

if [ -z "$debug_bucket" ]; then
  aws s3 cp "$vidir" "$vi_bucket_key" --exclude "*" --include "*.tif" \
    --include "*.xml" --include "*.jpg" --include "*_stac.json" \
    --profile gccprofile --recursive

  # Copy vi manifest to S3 to signal completion.
  aws s3 cp "$vi_manifest" "${vi_bucket_key}/${vi_manifest_name}" --profile gccprofile
else
  # Copy all vi files to debug bucket.
  echo "Copy files to debug bucket"
  debug_bucket_key=s3://${debug_bucket}/${outputname}
  aws s3 cp "$vidir" "$debug_bucket_key" --recursive --acl public-read
fi

