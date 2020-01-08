#!/bin/bash

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

# Create array from granulelist
IFS=','
read -r -a granules <<< "$granulelist"
# Consolidate twin granules if necessary, if not rename the granule ouput.
if [ "${#granules[@]}" = 2 ]; then
  # Use the base SAFE name without the unique id for the output file name.
	IFS='_'
	read -ra granulename <<< "${granules[0]}"
  outputname="${granulename[0]}_${granulename[1]}_${granulename[2]}_${granulename[3]}_${granulename[4]}_${granulename[5]}"
  # Process each granule in granulelist
  consolidatelist=()
  for granule in "${granules[@]}"; do
    granuleoutput=$(source sentinel_granule.sh "$granule")
    consolidatelist+=(" ${granuleoutput}")
  done
  echo "Running consolidate on ${consolidatelist[@]}"
  consolidate "${consolidatelist[@]}" "${workingdir}/${outputname}.hdf"
else
	IFS='_'
	read -ra granulename <<< "$granulelist"
  outputname="${granulename[0]}_${granulename[1]}_${granulename[2]}_${granulename[3]}_${granulename[4]}_${granulename[5]}"
  granuleoutput=$(source sentinel_granule.sh "$granule")
  mv granuleoutput "${workingdir}/${outputname}.hdf"
fi

aws s3 copy "${workingdir}/${outputname}.hdf" "s3://${bucket}/${outputname}/${outputname}.hdf"
