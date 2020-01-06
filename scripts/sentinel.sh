#!/bin/bash

# Exit on any error
set -o errexit
jobid="$AWS_BATCH_JOB_ID"
granulelist="$GRANULE_LIST"
workingdir="/tmp/${jobid}"

# Remove tmp files on exit
trap "rm -rf $workingdir; exit" INT TERM EXIT

# Create workingdir
mkdir -p $workingdir

# Create array from granulelist
IFS=',' read -r -a granules <<< "$granulelist"

consolidatelist=()
# Process each granule in granulelist
for granule in "${granules[@]}"
do
  granuleoutput=$(source sentinel_granule.sh "$granule")
  consolidatelist+=(" ${granuleoutput}")
done

# Consolidate twin granules if necessary
if [ ${#consolidatelist[@]} = 2 ]; then
  IFS='_'
  # Read into an array as tokens separated by IFS
  read -ra granulename <<< "$id"

  consolidatedname = "${workingdir}/${granulename[0]}_${granulename[1]}_${granulename[2]}_${granulename[3]}_${granulename[4]}_${granulename[5]}.hdf"
  consolidate "${consolidatelist[@]}" "$consolidatedname"
fi
