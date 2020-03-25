#/bin/bash

# Exit on any error
set -o errexit

jobid="$AWS_BATCH_JOB_ID"
granulelist="$GRANULE_LIST"
bucket=$OUTPUT_BUCKET
inputbucket=$INPUT_BUCKET
workingdir="/tmp/${jobid}"

# Remove tmp files on exit
# trap "rm -rf $workingdir; exit" INT TERM EXIT

# # Create workingdir
# mkdir -p "$workingdir"

# The derive_s2nbar C code infers values from the input file name so this
# formatting is necessary.  This implicit name requirement is not documented
# anywhere!

# The required format for nbar is read in sections from left to right
# HLS.S30.${tileid}.${year}${doy}.${obs}.nbar.${HLSVER}.hdr
set_output_names () {
  # Use the base SAFE name without the unique id for the output file name.
	IFS='_'
	read -ra granulecomponents <<< "$1"

  date=${granulecomponents[2]:0:8}
  year=${date:0:4}
  month=${date:4:2}
  day=${date:6:2}
  day_of_year=$(get_doy.py -y "${year}" -m "${month}" -d "${day}")
  hms=${date:8:6}
  day_of_year=$(get_doy.py -y "${year}" -m "${month}" -d "${day}")
  outputname="HLS.S30.${granulecomponents[5]}.${year}${day_of_year}.${hms}.v1.5"
  nbar_input="${workingdir}/${outputname}.hdf"
  # We also need to obtain the sensor for the Bandpass parameters file
  sensor="${granulecomponents[0]:0:3}"
}

echo "Start processing granules"
# Create array from granulelist
IFS=','
read -r -a granules <<< "$granulelist"
# Consolidate twin granules if necessary.
if [ "${#granules[@]}" = 2 ]; then
  # Use the base SAFE name without the unique id for the output file name.
  set_output_names "${granules[0]}"
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

# Convert to COGs
echo "Converting to COGs"
hdf_to_cog.py "$nbar_input" --output-dir "$workingdir"

bucket_key="s3://${bucket}/${outputname}/" 

# Generate manifest
echo "Generating manifest"
manifest_name="${outputname}.json"
manifest="${workingdir}/${manifest_name}" 
create_manifest.py -i "$workingdir" -o "$manifest" -b "$bucket_key" -c "HLSS30"

# Copy output to S3.
aws s3 sync "$workingdir" "$bucket_key" --exclude "*" --include "*.tif" --include "*.xml" --include "*.jpg"

# Copy manifest to S3 to signal completion.
aws s3 cp "$manifest" "s3://${bucket_key}/${manifest_name}" 
