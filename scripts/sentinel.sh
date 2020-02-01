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

echo "Start processing granules"
# Create array from granulelist
IFS=','
read -r -a granules <<< "$granulelist"
# Consolidate twin granules if necessary, if not rename the granule ouput.
if [ "${#granules[@]}" = 2 ]; then
  # Use the base SAFE name without the unique id for the output file name.
	IFS='_'
	read -ra granulename <<< "${granules[0]}"
  outputname="${granulename[0]}_${granulename[1]}_${granulename[2]}_${granulename[3]}_${granulename[4]}_${granulename[5]}"
  # Process each granule in granulelist and build the consolidatelist
  consolidatelist=""
  for granule in "${granules[@]}"; do
    source sentinel_granule.sh
    granuleoutput="${workingdir}/${granule}/${granule}_sr_output.hdf"
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
	IFS='_'
	read -ra granulename <<< "$granulelist"
  outputname="${granulename[0]}_${granulename[1]}_${granulename[2]}_${granulename[3]}_${granulename[4]}_${granulename[5]}"
  granuleoutput=$(source sentinel_granule.sh "$granulelist")
  mv granuleoutput "${workingdir}/${outputname}.hdf"
fi

aws s3 cp "${workingdir}/${outputname}.hdf" "s3://${bucket}/${outputname}/${outputname}.hdf"
