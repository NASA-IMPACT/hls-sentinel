#/bin/bash

# Exit on any error
set -o errexit

jobid="$AWS_BATCH_JOB_ID"
granulelist="$GRANULE_LIST"
bucket="$OUTPUT_BUCKET"
inputbucket="$INPUT_BUCKET"
workingdir="/tmp/${jobid}"
bucket_role_arn="$GCC_ROLE_ARN"
debug_bucket="$DEBUG_BUCKET"
replace_existing="$REPLACE_EXISTING"

# Remove tmp files on exit
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
  twinkey=""
  if [ ! -z "$2" ]; then
    twinkey="/twin"
  fi
  date=${granulecomponents[2]:0:15}
  year=${date:0:4}
  month=${date:4:2}
  day=${date:6:2}
  hms=${date:8:7}

  day_of_year=$(get_doy.py -y "${year}" -m "${month}" -d "${day}")
  outputname="HLS.S30.${granulecomponents[5]}.${year}${day_of_year}${hms}.v1.5"
  output_hdf="${workingdir}/${outputname}.hdf"
  nbar_name="HLS.S30.${granulecomponents[5]}.${year}${day_of_year}.${hms}.v1.5"
  nbar_input="${workingdir}/${nbar_name}.hdf"
  nbar_hdr="${nbar_input}.hdr"
  output_thumbnail="${workingdir}/${outputname}.jpg"
  output_metadata="${workingdir}/${outputname}.cmr.xml"
  bucket_key="s3://${bucket}/S30/data/${year}${day_of_year}/${outputname}${twinkey}"

  # We also need to obtain the sensor for the Bandpass parameters file
  sensor="${granulecomponents[0]:0:3}"
}

exit_if_exists () {
  if [ ! -z "$replace_existing" ]; then
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
mv "$resample30m" "$nbar_input"
mv "$resample30m_hdr" "$nbar_hdr"

# Nbar
echo "Running derive_s2nbar"
cfactor="${workingdir}/cfactor.hdf"
derive_s2nbar "$nbar_input" "$angleoutput" "$cfactor"

# Bandpass
echo "Running L8like"
parameter="/usr/local/bandpass_parameter.${sensor}.txt"
L8like "$parameter" "$nbar_input"

mv "$nbar_input" "$output_hdf"
mv "${nbar_input}.hdr" "${output_hdf}.hdr"

# Convert to COGs
echo "Converting to COGs"
hdf_to_cog.py "$output_hdf" --output-dir "$workingdir"

# Create thumbnail
echo "Creating thumbnail"
create_thumbnail -i "$output_hdf" -o "$output_thumbnail" -s S30

# Create metadata
echo "Creating metadata"
create_metadata "$output_hdf" --save "$output_metadata"

# Generate manifest
echo "Generating manifest"
manifest_name="${outputname}.json"
manifest="${workingdir}/${manifest_name}"
create_manifest.py -i "$workingdir" -o "$manifest" -b "$bucket_key" -c "HLSS30" -p "$outputname" -j "$jobid"

# Copy output to S3.
mkdir -p ~/.aws
echo "[profile gccprofile]" > ~/.aws/config
echo "region=us-east-1" >> ~/.aws/config
echo "output=text" >> ~/.aws/config

echo "[gccprofile]" > ~/.aws/credentials
echo "role_arn = ${GCC_ROLE_ARN}" >> ~/.aws/credentials
echo "credential_source = Ec2InstanceMetadata" >> ~/.aws/credentials

if [ -z "$debug_bucket" ]; then
  aws s3 cp "$workingdir" "$bucket_key" --exclude "*" --include "*.tif" \
    --include "*.xml" --include "*.jpg" --exclude "*fmask.bin.aux.xml" \
    --profile gccprofile --recursive

  # Copy manifest to S3 to signal completion.
  aws s3 cp "$manifest" "${bucket_key}/${manifest_name}" --profile gccprofile
else
  # Copy all intermediate files to debug bucket.
  debug_bucket_key=s3://${debug_bucket}/${outputname}
  aws s3 cp "$workingdir" "$debug_bucket_key" --recursive
fi
