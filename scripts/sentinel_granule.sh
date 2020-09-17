#!/bin/bash
# shellcheck disable=SC2154

# Exit on any error
set -o errexit

# granule, granuledir, inputbucket, angleoutput, granuleoutput variable set in sentinel.sh
safedirectory="${granuledir}/${granule}.SAFE"
safezip="${granuledir}/${granule}.zip"
inputgranule="s3://${inputbucket}/${granule}.zip"

# Intermediate outputs.
fmaskbin="${granuledir}/fmask.bin"
detfoo="${granuledir}/detfoo.hdf"
mkdir -p "$granuledir"

# IFS='_'
# # Read into an array as tokens separated by IFS
# read -ra ADDR <<< "$granule"

# Format GCS url and download
# url=gs://gcp-public-data-sentinel-2/tiles/${ADDR[5]:1:2}/${ADDR[5]:3:1}/${ADDR[5]:4:2}/${granule}.SAFE
# gsutil -m cp -r "$url" "$granuledir"

# Download granule from s3
aws s3 cp "$inputgranule" "$safezip"
unzip -q "$safezip" -d "$granuledir"

# Get GRANULE sub directory
grandir_id=$(get_s2_granule_dir "${safedirectory}")
safegranuledir="${safedirectory}/GRANULE/${grandir_id}"

# Run derive_s2ang

# For new SAFE format locate MTD_MSIL1C.xml
# For older SAFE formata locate S2[A|B]_OPER_MTD_*.xml
xml=$(find "$safegranuledir" -maxdepth 1 -type f -name "*.xml")

# Check solar zenith angle.
echo "Check solar azimuth"
solar_zenith_valid=$(check_solar_zenith_sentinel "$xml")
if [ "$solar_zenith_valid" == "invalid" ]; then
  echo "Invalid solar zenith angle. Exiting now"
  exit 3
fi

# Locate DETFOO gml for Band 01.  derive_s2ang will then infer names for other bands
detfoo_gml=$(find "${safegranuledir}/QI_DATA" -maxdepth 1 -type f -name "*DETFOO*_B01*.gml")
echo "Running derive_s2ang"
derive_s2ang "$xml" "$detfoo_gml" "$detfoo" "$angleoutput"

# The detfoo output is an unneccesary legacy output
rm "$detfoo"

cd "$safegranuledir"
# Run Fmask
/usr/local/MATLAB/application/run_Fmask_4_2.sh /usr/local/MATLAB/v96 >> fmask_out.txt
fmask_stdout=$(tail -4 fmask_out.txt)
echo "$fmask_stdout"
echo "Running parse_fmask"
fmask_valid=$(parse_fmask "$fmask_stdout")
if [ "$fmask_valid" == "invalid" ]; then
  echo "Fmask reports no clear pixels. Exiting now"
  exit 4
fi

fmask="${safegranuledir}/FMASK_DATA/${grandir_id}_Fmask4.tif"

# Convert to flat binary
gdal_translate -of ENVI "$fmask" "$fmaskbin"

# Zips and unpacks S2 SAFE directory.  The ESA SAFE data will be provided zipped.
cd "$granuledir"
# zip -r -q "$safezip" "${granule}.SAFE"

# Removes previously unzipped SAFE directory for replacement with ESPA unpacking
# result
rm -rf "${granule}.SAFE"

unpackage_s2.py -i "$safezip" -o "$granuledir"
rm "$safezip"

# Convert to espa format
cd "$safedirectory"
convert_sentinel_to_espa

# After conversion remove all Sentinel jp2 files to reduce disk usage.
if [ -z "$debug_bucket" ]; then
  rm ./*.jp2
fi

# Get the new granule id generated by the ESPA conversion
# shellcheck disable=SC1083
espa_xml=$(find . -type f -name "*.xml" ! -name "MTD_TL.xml" ! -name "MTD_MSIL1C.xml" -exec basename \{} \;)
espa_id="${espa_xml%.*}"

# Run lasrc
do_lasrc_sentinel.py --xml "$espa_xml"

hls_espa_one_xml="${espa_id}_1_hls.xml"
hls_espa_two_xml="${espa_id}_2_hls.xml"
sr_hdf_one="${espa_id}_sr_1.hdf"
sr_hdf_two="${espa_id}_sr_2.hdf"
hls_sr_combined_hdf="${espa_id}_sr_combined.hdf"
# Surface reflectance is current final output
hls_sr_output_hdf="$granuleoutput"

# Create ESPA xml files using HLS v1.5 band names.
create_sr_hdf_xml "$espa_xml" "$hls_espa_one_xml" one
create_sr_hdf_xml "$espa_xml" "$hls_espa_two_xml" two

# Convert ESPA xml files to HDF
convert_espa_to_hdf --xml="$hls_espa_one_xml" --hdf="$sr_hdf_one"
convert_espa_to_hdf --xml="$hls_espa_two_xml" --hdf="$sr_hdf_two"

# Combine split hdf files and resample 10M SR bands back to 20M and 60M.
echo "Combining hdf files"
twohdf2one "$sr_hdf_one" "$sr_hdf_two" MTD_MSIL1C.xml MTD_TL.xml LaSRC "$hls_sr_combined_hdf"

# Run addFmaskSDS
echo "Adding Fmask SDS"
addFmaskSDS "$hls_sr_combined_hdf" "$fmaskbin" MTD_MSIL1C.xml MTD_TL.xml LaSRC "$hls_sr_output_hdf"

# Trim edge pixels for spurious SR values
echo "Trimming output hdf file"
s2trim "$hls_sr_output_hdf"

# Remove intermediate files.
cd "$granuledir"
# Keep all intermediate files in debug mode
if [ -z "$debug_bucket" ]; then
  rm -rf "$safedirectory"
fi
