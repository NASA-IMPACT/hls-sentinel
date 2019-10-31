#!/bin/bash

# id="S2B_MSIL1C_20190902T184919_N0208_R113_T11UNQ_20190902T222415"
id=$SENTINEL_SCENE_ID
bucket=$OUTPUT_BUCKET
IFS='_'

# Read into an array as tokens separated by IFS
read -ra ADDR <<< "$id"

# Format GCS url and download
url=gs://gcp-public-data-sentinel-2/tiles/${ADDR[5]:1:2}/${ADDR[5]:3:1}/${ADDR[5]:4:2}/${id}.SAFE/
gsutil -m cp -r "$url" /tmp

# Move metadata files to IMG_DATA for ESAP conversion
cd "/tmp/${id}.SAFE/GRANULE"
granule_id=$(find . -maxdepth 1 -type d -name '[^.]?*' -printf %f -quit)
workingdir=/tmp/${id}.SAFE/GRANULE/${granule_id}/IMG_DATA/
cp "/tmp/${id}.SAFE/MTD_MSIL1C.xml" "$workingdir"
cp "/tmp/${id}.SAFE/GRANULE/${granule_id}/MTD_TL.xml" "$workingdir"

cd "$workingdir"
# Convert to espa format
convert_sentinel_to_espa

# Run lasrc
do_lasrc_s2.py --xml "${granule_id}.xml"

if [ $? -ne 0 ]
then
  echo "Lasrc failed"
else
  convert_espa_to_hdf --xml "${id}.xml" --hdf  "${id}_sr.hdf"
  aws s3 sync . "s3://${bucket}/"
fi

