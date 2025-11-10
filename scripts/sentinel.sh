#!/bin/bash
# shellcheck disable=SC2153
# shellcheck disable=SC1091
# Exit on any error
set -o errexit

save_debug_output="$SAVE_DEBUG_OUTPUT"
exit_after_fmask="$EXIT_AFTER_FMASK"
exit_after_lasrc="$EXIT_AFTER_LASRC"


#granulelist="S2A_MSIL1C_20180213T110141_N0500_R094_T31UDR_20230907T125643"
#"S1A_MSIL1C_20250408T001241_N0511_R073_T55HGB_20250408T030123"
granulelist="$GRANULE_LIST"
echo $granulelist
workingdir="/var/scratch"
outputdir="/tmp/output"

if [ "$exit_after_lasrc" == "true" ] && [ "$exit_after_fmask" == "true" ]; then
   echo "Set exit_after_fmask  to false to proceed"
   exit
fi

replace_existing="$REPLACE_EXISTING"
echo "$replace_existing"

# Remove tmp files on exit
# shellcheck disable=SC2064
trap "rm -rf $workingdir; exit" INT TERM EXIT

# Create workingdir
mkdir -p "$workingdir"

# Create outputdir
mkdir -p "$outputdir"

# The derive_s2nbar C code infers values from the input file name so this
# formatting is necessary.  This implicit name requirement is not documented
# anywhere!

# The required format for nbar is read in sections from left to right
# HLS.S30.${tileid}.${year}${doy}.${obs}.nbar.${HLSVER}.hdr
set_output_names () {
  # Use the base SAFE name without the unique id for the output file name.
  IFS='_'
  read -ra granulecomponents <<< "$1"
  echo $granulecomponents
  # Include twin in bucket key for s3 when argument is included.
  # this is necessary for LPDAAC's ingestion timing.
  twinkey=""
  if [ -n "$2" ]; then
    twinkey="/twin"
  fi
  date=${granulecomponents[2]:0:15}
  year=${date:0:4}
  month=${date:4:2}
  day=${date:6:2}
  hms=${date:8:7}

  hlsversion="v2.0"
  fmaskversion="4.7"
  day_of_year=$(get_doy "${year}" "${month}" "${day}")
  outputname="HLS.S30.${granulecomponents[5]}.${year}${day_of_year}${hms}.${hlsversion}"
  output_hdf="${workingdir}/${outputname}.hdf"
  nbar_name="HLS.S30.${granulecomponents[5]}.${year}${day_of_year}.${hms}.${hlsversion}"
  nbar_input="${workingdir}/${nbar_name}.hdf"
  nbar_hdr="${nbar_input}.hdr"
  output_thumbnail="${workingdir}/${outputname}.jpg"
  output_metadata="${workingdir}/${outputname}.cmr.xml"
  output_stac_metadata="${workingdir}/${outputname}_stac.json"
  # We also need to obtain the sensor for the Bandpass parameters file
  sensor="${granulecomponents[0]:0:3}"
  angleoutputfinal="${workingdir}/${outputname}.ANGLE.hdf"
  IFS=''
}

exit_if_exists () {
  if [ -n "$replace_existing" ]; then
    # Check if output folder key exists
    exists=$(ls "${outputdir}/" | wc -l)
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
echo $granules
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
  echo $granule
  set_output_names "$granule"
  exit_if_exists
  

  granuledir="${workingdir}/${granule}"
  angleoutput="${granuledir}/angle.hdf"
  granuleoutput="${granuledir}/sr.hdf"
  echo "start"  
  source sentinel_granule.sh
fi

# Resample to 30m
echo "Running create_s2at30m"
resample30m="${workingdir}/resample30m.hdf"
resample30m_hdr="${resample30m}.hdr"
create_s2at30m "$granuleoutput" "$resample30m"

# Unlike all the other C libs, derive_s2nbar and L8like modify the input file
# Move the resample output to nbar naming.
# Maintain intermediate 30m version in debug mode.
if [ "save_debug_output" == "false" ]; then
  mv "$resample30m" "$nbar_input"
  mv "$resample30m_hdr" "$nbar_hdr"
else
  cp "$resample30m" "$nbar_input"
  cp "$resample30m_hdr" "$nbar_hdr"
fi

if [ "$exit_after_lasrc" == "true" ]; then
  rsync -av ${workingdir}/ "${outputdir}/${outputname}/"
  echo "LaSRC successfully completed. Saving resampled output to $outputdir. Exiting now"
  exit
fi

# Nbar
echo "Running derive_s2nbar"
cfactor="${workingdir}/cfactor.hdf"
derive_s2nbar "$nbar_input" "$angleoutput" "$cfactor"

nbarIntermediate="${workingdir}/nbarIntermediate.hdf"
nbarIntermediate_hdr="${nbarIntermediate}.hdr"
# Maintain intermediate nbar version in debug mode.
if [ "$save_debug_output" == "true" ]; then
  cp "$nbar_input" "$nbarIntermediate"
  cp "$nbar_hdr" "$nbarIntermediate_hdr"
fi

# Bandpass
echo "Running L8like"
parameter="/usr/local/bandpass_parameter.${sensor}.txt"
L8like "$parameter" "$nbar_input"

mv "$nbar_input" "$output_hdf"
mv "${nbar_input}.hdr" "${output_hdf}.hdr"

# Convert to COGs
echo "Converting to COGs"
hdf_to_cog "$output_hdf" --output-dir "$workingdir" --product S30

mv "$angleoutput" "$angleoutputfinal"
hdf_to_cog "$angleoutputfinal" --output-dir "$workingdir" --product S30_ANGLES

# Create thumbnail
echo "Creating thumbnail"
create_thumbnail -i "$workingdir" -o "$output_thumbnail" -s S30

# Create metadata
echo "Creating metadata"
create_metadata "$output_hdf" --save "$output_metadata"

# Create STAC metadata
cmr_to_stac_item "$output_metadata" "$output_stac_metadata" \
  data.lpdaac.earthdatacloud.nasa.gov 020

# Generate manifest
echo "Generating manifest"
manifest_name="${outputname}.json"
manifest="${workingdir}/${manifest_name}"
create_manifest "$workingdir" "$manifest" "$bucket_key" "HLSS30" \
  "$outputname" "$jobid" false


if [ "$save_debug_output" == "false" ]; then
  mkdir -p "${outputdir}/${outputname}/"
  rsync -av --include="*.tif" \
    --include="*.xml" --include="*.jpg" --include="*_stac.json" \
    --exclude="*fmask.bin.aux.xml" --exclude="*" "${workingdir}/" "/tmp/output/${outputname}/" 

else
  # Create
  # Convert intermediate hdf to COGs
  hdf_to_cog "$resample30m" --output-dir "$workingdir" --product S30 --debug-mode
  hdf_to_cog "$nbarIntermediate" --output-dir "$workingdir" --product S30 --debug-mode

  # Copy all intermediate files to debug bucket.
  echo "Copy files to debug bucket"
  mkdir -p "${outputdir}/${outputname}/"
  rsync -av "${workingdir}/" "/tmp/output/${outputname}/" 
fi

#done
echo "All files created"

