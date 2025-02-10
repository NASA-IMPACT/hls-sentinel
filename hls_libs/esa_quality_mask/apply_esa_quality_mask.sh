# Note:  insert the "esa_quality_mask" code right after "addFmaskSDS" (before "s2trim")

########## What comes before:
# addFmaskSDS $out_noMASK $fmask20mbin $safexml $granxml $ACCODE $outfile

########## esa_quality_mask
# Use ESA's image quality mask to filter data.
# The raster mask is available only for processing baseline 4.0 and above (roughly
# images produced after January 2022). For earlier processing baseline, nothing to do.
# 
# Test if the quality mask exists.
# Default is that it exists.
ESA_Quality_Mask=true
for  band in 01 02 03 04 05 06 07 08 8A 09 10 11 12	
do 
	# Convert mask from JP2 to ENVI format for each band. Only record the ENVI B01
	# filename for later use by the C code, which applies the masks for all bands, 
	# by inferring mask filename of other bands from B01 mask filename.
	#
	# The variable grandir refers to a SAFE directory name.
	maskjp2=$(ls $grandir/GRANULE/*/QI_DATA/MSK_QUALIT_B${band}.jp2 2>/dev/null)
	if [ $? -ne 0 ]
	then
		# Processing baseline older than 4.0; no quality mask in JP2. 
		ESA_Quality_Mask=false
		break
	fi
	jp2base=$(basename $maskjp2)
	maskenvi=$tmpdir/$jp2base.envi

	gdal_translate -of ENVI $maskjp2 $maskenvi

	# Record the filename of the converted B01 quality mask; the filenames of any other band
	# is inferred from this. 
	case $band in
	  01) maskb01=$maskenvi;;
	esac
done

# If quality mask exists, apply it. 
if $ESA_Quality_Mask
then
	# Apply mask for all bands; only B01 filename is given as argument.
	# outfile is the output from the above "addFmaskSDS" and is updated in place.
	$CODE_BASE_DIR/S2/toTOA/ESA_Quality_Mask/esa_quality_mask $outfile $maskb01
fi

