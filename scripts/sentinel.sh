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
  for granule in "${granules[@]}"; do
    granuledir="${workingdir}/${granule}"
    angleoutput="${granuledir}/angle.hdf"
    granuleoutput="${granuledir}/output.hdf"
    source sentinel_granule.sh
    if [ "${#consolidatelist}" = 0 ]; then
      consolidatelist="${granuleoutput}"
    else
      consolidatelist="${consolidatelist} ${granuleoutput}"
    fi
  done
  echo "Running consolidate on ${consolidatelist}"
  consolidate_command="consolidate ${consolidatelist} ${workingdir}/${outputname}.hdf"
  eval "$consolidate_command"
else
  # If it is a single granule, just copy the granule output and do not consolidate
  granule="$granulelist"
  set_outputname "$granule"

  granuledir="${workingdir}/${granule}"
  angleoutput="${granuledir}/angle.hdf"
  granuleoutput="${granuledir}/output.hdf"

  source sentinel_granule.sh
  mv "$granuleoutput" "${workingdir}/${outputname}.hdf"
  mv "$angleoutput" "${workingdir}/${outputname}_angle.hdf"
fi

aws s3 cp "${workingdir}/${outputname}.hdf" "s3://${bucket}/${outputname}/${outputname}.hdf"
aws s3 cp "${workingdir}/${outputname}_angle.hdf" "s3://${bucket}/${outputname}/${outputname}_angle.hdf"
