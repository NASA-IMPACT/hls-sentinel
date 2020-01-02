#!/bin/bash

# Exit on any error
set -o errexit
jobid="$AWS_BATCH_JOB_ID"
granulelist="$GRANULE_LIST"
workingdir="/tmp/${jobid}"

# Remove tmp files on exit
trap "rm -rf $workingdir; exit" INT TERM EXIT

# Create workingdir
mkdir "$workingdir"

# Create array from granulelist
IFS=',' read -r -a granules <<< "$granulelist"

# Process each granule in granulelist
for granule in "${granules[@]}"
do
   source sentinel_granule.sh "$granule"
done
