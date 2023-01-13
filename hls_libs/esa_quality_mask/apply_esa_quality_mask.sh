########## Use ESA's image quality mask to filter data.
# The raster mask is available only for processing baseline 4.0 and above (roughly
# images produced after January 2022).

# Default is that the quality mask exists.
ESA_Quality_Mask=true
for  band in 01 02 03 04 05 06 07 08 8A 09 10 11 12
do 
	# Convert mask from JP2 to ENVI format for each band. Only record the ENVI B01
	# filename for later use.  The C code applying the masks for all bands, inferring 
	# mask filename of other bands from B01 filename.
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
if $ESA_Quality_Mask
then
	# Apply mask for all bands; only B01 filename is given as argument.
	$CODE_BASE_DIR/S2/toTOA/ESA_Quality_Mask/esa_quality_mask $outfile $maskb01
fi
