#/bin/bash

# Exit on any error
set -o errexit

jobid="$AWS_BATCH_JOB_ID"
granulelist="$GRANULE_LIST"
bucket=$OUTPUT_BUCKET
workingdir="/tmp/${jobid}"

# Remove tmp files on exit
trap "rm -rf $workingdir; exit" INT TERM EXIT

# Create workingdir
mkdir -p $workingdir

set_outputname () {
  # Use the base SAFE name without the unique id for the output file name.
	IFS='_'
	read -ra granulecomponents <<< "$1"
  outputname="${granulecomponents[0]}_${granulecomponents[1]}_${granulecomponents[2]}_${granulecomponents[3]}_${granulecomponents[4]}_${granulecomponents[5]}"
}

echo "Start processing granules"
# Create array from granulelist
IFS=','
read -r -a granules <<< "$granulelist"
# Consolidate twin granules if necessary.
if [ "${#granules[@]}" = 2 ]; then
  # Use the base SAFE name without the unique id for the output file name.
  set_outputname "${granules[0]}"

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
  consolidate_output="${workingdir}/${outputname}_consolidate.hdf"
  consolidate_angle_output="${workingdir}/${outputname}_consolidate_angle.hdf"
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
  set_outputname "$granule"

  granuledir="${workingdir}/${granule}"
  angleoutput="${granuledir}/angle.hdf"
  granuleoutput="${granuledir}/sr.hdf"

  source sentinel_granule.sh
fi

output="${workingdir}/${outputname}.hdf"
# Resample to 30m
create_s2_at30m "$granuleoutput" "${output}"

aws s3 cp "${output}" "s3://${bucket}/${outputname}/${outputname}.hdf"
aws s3 cp "${angleoutput}" "s3://${bucket}/${outputname}/${outputname}_angle.hdf"
