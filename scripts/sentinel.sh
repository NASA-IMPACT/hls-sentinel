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
mkdir -p "$workingdir"

set_outputname () {
  # Use the base SAFE name without the unique id for the output file name.
	IFS='_'
	read -ra granulecomponents <<< "$1"
  outputname="${granulecomponents[0]}_${granulecomponents[1]}_${granulecomponents[2]}_${granulecomponents[3]}_${granulecomponents[4]}_${granulecomponents[5]}"
}

# The derive_s2nbar C code infers values from the input file name so this
# formatting is necessary.  This implicit name requirement is not documented
# anywhere!
set_nbar_input () {
	IFS='_'
	read -ra granulecomponents <<< "$1"
  # The required format is
  # HLS.S30.T${tileid}.${year}${doy}.${obs}.nbar.${HLSVER}.hdf
  nbar_input="${workingdir}/HLS.S30.T${granulecomponents[5]}.${granulecomponents[2]:0:8}.${granulecomponents[6]}.nbar.1.5.hdf"
}

set_bandpass_output_name () {
	IFS='_'
	read -ra granulecomponents <<< "$1"
  bandpass_output_name="HLS.S30.T${granulecomponents[5]}.${granulecomponents[2]:0:8}.${granulecomponents[6]}.1.5.hdf"
}

echo "Start processing granules"
# Create array from granulelist
IFS=','
read -r -a granules <<< "$granulelist"
# Consolidate twin granules if necessary.
if [ "${#granules[@]}" = 2 ]; then
  # Use the base SAFE name without the unique id for the output file name.
  set_outputname "${granules[0]}"
  set_nbar_input "${granules[0]}"
  set_bandpass_output_name "${granules[0]}"
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
  set_nbar_input "$granule"
  set_bandpass_output_name "$granule"

  granuledir="${workingdir}/${granule}"
  angleoutput="${granuledir}/angle.hdf"
  granuleoutput="${granuledir}/sr.hdf"

  source sentinel_granule.sh
fi

# Resample to 30m
echo "Running create_s2at30m"
resample30m="${workingdir}/${outputname}_resample30m.hdf"
create_s2at30m "$granuleoutput" "$resample30m"

# Move the resample output to nbar naming.
mv "$resample30m" "$nbar_input"

# Nbar
echo "Running derive_s2nbar"
cfactor="${workingdir}/cfactor.hdf"
derive_s2nbar "$nbar_input" "$angleoutput" "$cfactor"

# Bandpass
echo "Running L8like"
sensor="${outputname:0:3}"
parameter="/usr/local/bandpass_parameter.${sensor}.txt"
L8like "$parameter" "$nbar_input"

aws s3 cp "$nbar_input" "s3://${bucket}/${outputname}/${bandpass_output_name}"
