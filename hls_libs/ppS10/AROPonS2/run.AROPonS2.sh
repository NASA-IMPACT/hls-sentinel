#!/bin/bash

################################################################################
# For the given AC output on a tile, find the corresponding reference image and
# run AROP on the two images to get the fitting result.
#
# Input::
#	An S10 surface reflectance hdf file, which is the only commandline parameter.
#       The reference image for a tile has been selected beforehand and can be found
# 	by this script.
#
#	An example basename of input filename:  
# 	  HLS.S10.T56HKH.2016181.consolidated.hdf	# consolidated 
#	  HLS.S10.T11SPC.2016260.v1.2.hdf		#  	
# Output::
#       AROP fitting result (captured stdout), control point file, and the input 
#	parameter file; all three are saved to a permanent directory for later 
# 	use if necessary.
#
#	The return is the filename of the fitting result echoed to stdout, to be
# 	used by the caller.
#
# Exit status::
#	 0 for success, 
#        1 for failure.
#	 2 for other error, such as the executable not found.
#
# Dec 15, 2016 
# Dec 23, 2016
################################################################################

if [ $# -ne 1 ]
then
	echo "$0  S10.hdf " >&2
	exit 1
fi
s10hdf=$1

for var in S2REFIMG_DIR \
	   CODE_BASE_DIR \
	   AROP_INI_S2 \
	   LOCAL_IO_DIR \
	   TDIR 
do
	val=$(eval echo \$$var)
	if [ -z "$val" ]
	then
		echo "$var not set" >&2
		echo "Line $LINENO of ${BASH_SOURCE[0]}" >&2
		exit 1
	fi
done

set -u 
set -o pipefail

### Parse input filename for tileid. Check the validity of the filename.
# Example basename: HLS.S10.T56HKH.2016181.consolidated.hdf
# Example basename: HLS.S10.T56HKH.2016181.hdf
# Beause AROP uses the term "base image" for reference image,  this script calls
# the output from command "basename" bname. 
warp_bname=$(basename $s10hdf .hdf)
tileid=$(echo $warp_bname | 
  awk -F"." '{ 	Ttileid = $3; 
		if (Ttileid ~ /^T[0-9][0-9][A-Z][A-Z][A-Z]$/)
			print substr(Ttileid,2,5)
		else 
			print "NONE"
	    }')
if [ $tileid = "NONE" ]
then
	echo "The filename is not of naming convention: $s10hdf" >&2
	exit 1
fi

# Used in naming output directory.
year=$(  echo $warp_bname | awk -F"." '{print substr($4,1,4)}' ) 

tmpdir=$TDIR/S2/ppS10/AROPonS2/$tileid
mkdir -p $tmpdir || exit 1

### Because this script returns something by echoing to stdout at the end, 
#   redirect stdout of the beginning part of the script to a file. Before 
#   "echo" at the end of the script for return, revert to the original stdout.
#   The content of this file does not matter, so the filename is not unique.
tmpstdout=$tmpdir/tmp.script.stdout.txt
exec 3>&1	# Save fd 1
exec 1>$tmpstdout

### Reference image (AROP calls it base image)
base_bin=$(ls $S2REFIMG_DIR/*${tileid}*_B08.bin)
if [ $? -ne 0 ]
then
	echo "No S2 referenc image for tile $tileid in $S2REFIMG_DIR" >&2
	echo "Line $LINENO of ${BASH_SOURCE[0]}" >&2 
	exit 1
fi
nimg=$(echo $base_bin | awk '{print NF}')
if [ $nimg -ne 1 ]
then
	echo "Wrong. There are $nimg reference image: $base_bin" >&2
	echo "Line $LINENO of ${BASH_SOURCE[0]}" >&2 
	exit 1
fi

### The base name of directory names for naming files.
base_bname=$(basename $base_bin .bin)

aropoutdir=$LOCAL_IO_DIR/S2/AROPlogdir/$year/$tileid
mkdir -p $aropoutdir || exit 1 
  param=$aropoutdir/${base_bname}_${warp_bname}.arop.inp
aropfit=$aropoutdir/${base_bname}_${warp_bname}.arop.stdout
aroplog=$aropoutdir/${base_bname}_${warp_bname}.arop.log
aroperr=$aropoutdir/${base_bname}_${warp_bname}.arop.stderr

### 
trap 'stat=$?; rm -f 	$param \
			$aropfit \
			$aroplog \
			$aroperr ; exit $stat'  1 2 15

warp_bin=
trap 'rm -f $warp_bin $tmpstdout' 0

### Image one: BASE

# Note that the new version of gdal gives warning --
# Warning 6: Driver ENVI does not support NBITS creation option. 
# Ralated to this, when converted to TIFF before atmospheric correction,
# NBITS must be specified as a new requirement to work correctly.
base_hdr=$(echo $base_bin | sed 's/bin$/hdr/')
base_nsample=$(awk '$1 ~ /samples/ {print $3}' $base_hdr)
base_nline=$(  awk '$1 ~ /lines/   {print $3}' $base_hdr)
set $( awk -F, '$0 ~ /map info/ {print $4, $5, $6, $8, $9}' $base_hdr)  
base_ulx=$1
base_uly=$2
base_pixsz=$3
base_zone=$4
base_hemisphere=$5

if [ "$base_hemisphere" = "South" ]	# Accommodate GCTP 
then
	base_uly=$(echo $base_uly | awk '{print $1 - 10000000}')
fi


### Image two: WARP
warp_bin=$tmpdir/tmp.warpbin.${base_bname}_${warp_bname}.bin
hdp dumpsds -n B08 -b -o $warp_bin $s10hdf  
if [ $? -ne 0 ]
then
	echo "Line $LINENO of ${BASH_SOURCE[0]}" >&2 
	exit 1
fi

# Image geometry same as base
warp_nsample=$base_nsample
warp_nline=$base_nline
warp_ulx=$base_ulx
warp_uly=$base_uly
warp_pixsz=$base_pixsz
warp_zone=$base_zone

cat << EOF > $param
PARAMETER_FILE

# use Sentinel2 MSI image (NIR band08 @10m ) as a base image
BASE_LANDSAT = $base_bin
BASE_FILE_TYPE = BINARY
BASE_DATA_TYPE = 2
BASE_NSAMPLE = $base_nsample
BASE_NLINE = $base_nline
BASE_PIXEL_SIZE = $base_pixsz
BASE_UPPER_LEFT_CORNER = $base_ulx, $base_uly
BASE_UTM_ZONE = $base_zone
BASE_DATUM = 12
# use Landsat1-5, Landsat7, Landsat8, TERRA, CBERS1, CBERS2, AWIFS, HJ1, Sentinel2 #
BASE_SATELLITE = Sentinel2

# Sentinel2 warp images 
WARP_NBANDS = 1
WARP_LANDSAT_BAND = $warp_bin
WARP_BAND_DATA_TYPE = 2
WARP_BASE_MATCH_BAND = $warp_bin
WARP_FILE_TYPE = BINARY
# following four variables (-1) will be read from warp match band if it's in GeoTIFF format 
WARP_NSAMPLE = $warp_nsample
WARP_NLINE = $warp_nline
WARP_PIXEL_SIZE = $warp_pixsz
WARP_UPPER_LEFT_CORNER = $warp_ulx, $warp_uly
WARP_UTM_ZONE = $warp_zone
WARP_DATUM = 12
# use Landsat1-5, Landsat7, Landsat8, TERRA, CBERS1, CBERS2, AWIFS, HJ1, Sentinel2 #
WARP_SATELLITE = Sentinel2 

# NN-nearest neighbor; BI-bilinear interpolation; CC-cubic convolution; AGG-aggregation; none-skip resampling #
RESAMPLE_METHOD = none
# BASE-use base map extent; WARP-use warp map extent; DEF-user defined extent # 
OUT_EXTENT = BASE
OUT_BASE_POLY_ORDER = 1
CP_LOG_FILE = $aroplog

# ancillary input for orthorectification process
CP_PARAMETERS_FILE = $AROP_INI_S2
END
EOF

# Run
# AROP writes a lot to the stderr and PBS does not like that; capture in a file.
>$aroperr
command=ortho
if [ ! -f $command ]
then
	echo "Command not found: $command" 2>>$aroperr
	exit 2
fi
$command -r $param > $aropfit  2>> $aroperr
if [ $? -ne 0 ]
then
	echo  >>$aroperr	# Add a blank line?
	echo "AROP failed for $param" >>$aroperr
	exit 1
fi

### It seems AROP always finishes "successuflly",  even if there is no output 
# in log due to error in input. So check log size
if [ ! -s $aroplog ]
then
	echo >>$aroperr
	echo "Failed: ortho -r $param" >>$aroperr
	echo "AROP log file is empty: $aroplog" >>$aroperr
	exit 1
fi

### If AROP is not certain, do not use
grep "matching test failed, use with CAUTION!!!" $aropfit >/dev/null 2>&1
if [ $? -eq 0 ]
then
	echo >>$aroperr
	echo "AROP is uncertain; do not use" >>$aroperr
	exit 1
fi

### Restore stdout
exec 1>&3
echo $aropfit
exit 0
