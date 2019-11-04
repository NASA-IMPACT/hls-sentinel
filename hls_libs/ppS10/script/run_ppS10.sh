#!/bin/bash 
###########################################################################
# Post-process rawS10 (output from atmospheric correction) if necessary.  
# The output is S10.
#
# At most two steps are needed for some granules, while nothing is needed
# for the other granules:  
#  1. If the atmospherically corrected twin granules (i.e., two same-day 
#     granules over the same tile location from two different data strips)
#     are present,  consolidate them into one granule; 
#  2. If the version of ithe granule is prior to processing baseline 02.04,
#     co-register it to a pre-selected reference image from processing baseline 02.04.
#
#  The command-line parameter for all S2 scripts in the processing chain is a
#  L1C granule directory, from which the path of the expected input can be 
#  derived.
#
#  If neither of the conditions exists for a granule, nothing is to be done
#  and rawS10 is directly moved into the S10 directory.
#
# Consolidation occurs before coregistration because a consolidated image
# may offer more tie points for coregistration than an individual twin image does. 
# While the warp image for coregistration is in surface reflectance, the reference
# image is absolutely fine to be in TOA reflectance. With respect to coregistration
# accuracy, it really does not matter whether TOA or BOA is used.  Reference image
# is in TOA reference because it is easy to select in data preparation. It would 
# be too much of a hassle to coregister a TOA image before atmospheric correction; 
# instead it is more convenient to coreigster the image after atmospheric correction.
#
# The twin granules comprise 3.1% of the input data. 
#
# Dec 28, 2016: The input parameter continues to be L1C "grandir", from which surface
# reflectance file is to be found. The identifier "A" or "B" is the last character of 
# the grandir name. If only A is present, only one granule. If B is present, twins are 
# present.  A lock file on the given tile ID and given day YYYYDDD is used to make sure, 
# if twin granules exist, only one process will consolidate the twins and register them
# to the reference image.  
#
# When a lock file is used to takes care of the twin-granule cases, all S2 scripts of
# the HLS processing chain can take input from the same granule list.
#
# Apr 21, 2017: Now if the input has any twin, both twins should have been included 
#      	(driver_script/find_new_granule.sh takes care of this), even if one of them has 
#	been processed before. And both twins will be logged.
# May 10, 2017:
# Aug 23, 2017: There is a chance that twins are assigned to the CPU at different
# 	times with a long interval in between. Log is inspected for the job submitted
#	on SUBDATE to avoid redo.
# May 31, 2018: The log information that originally passed through write_to_log now goes to stderr.
#
# Jun 26, 2018: Twin B is not included in the worklist, but the input is available (assuming atmospheric
#    correction is successful) is looked for by twin A. This is a workaround for the file locking
#    failure on cfncluster.
###########################################################################

if [ $# -ne 1 ]
then
	echo "Usage: $0 grandir" >&2
	exit 1
fi
grandir=$1

for var in CODE_BASE_DIR \
	   LOCAL_IO_DIR \
	   S2REFIMG_DIR \
	   S2LOGDIR \
	   JOBNAME \
	   SUBDATE \
	   TDIR \
	   HLSVER 
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

### Get the rawS10 pathname based on granule ID
# Example:
#    /ebs/1/S2L1C/2017/46/Q/E/H/S2B_OPER_PRD_MSIL1C_T46QEH_2017176A/
#    /ebs/1/S2L1C/2017/46/Q/E/H/S2B_OPER_PRD_MSIL1C_T46QEH_2017176A
# If the directory name has a trailing "/", remove it.
grandir=$(echo $grandir | sed 's:/$::')
granbase=$(basename $grandir)
set $(echo $granbase | awk -F"_"  '{ tileid = substr($(NF-1), 2,5)  
				     utmzone = substr(tileid, 1,2) 
				     utmband = substr(tileid, 3,1) 
				     gridx   = substr(tileid, 4,1) 
				     gridy   = substr(tileid, 5,1) 
	
				     year =   substr($NF, 1,4) 
				     doy =    substr($NF, 5,3) 
				     twinID = substr($NF, 8,1) 
				     print tileid, utmzone, utmband, gridx, gridy, year, doy, twinID
			     	   }' ) 
tileid=$1
utmzone=$2
utmband=$3
gridx=$4        
gridy=$5       
year=$6
doy=$7
twinID=$8	# A or B
branchdir=$year/$utmzone/$utmband/$gridx/$gridy

### Set up trap
outfile_con=		# an intermediate file, for "consolidated"
trap ' rm -f $outfile_con ${outfile_con}.hdr' 0

# Final S10
outdir=$LOCAL_IO_DIR/S2/S10/$branchdir
mkdir -p $outdir || exit 1
outfile_s10=$outdir/HLS.S10.T${tileid}.${year}${doy}.${HLSVER}.hdf
trap '	stat=$? 
	rm -f $outfile_s10 ${outfile_s10}.hdr 
	exit $stat' 1 2 15
# Delete any leftover.
rm -f ${outfile_s10}*

#################### Consolidate twins if present
### Use lock file to coordinate the processing for the rare cases of L1C twins.
### If there are twins for this tile for the given day, only one process will process
### them both.  (Two separate processes are assigned by the scheduler to the L1C twins,
### but we force one of them quit immediately.) If there are no twins, lock file can always 
### be successfully created, causing no inconvenience.
###
### If the two processes for the twins are scheduled to run sufficiently apart in time,
### the previous file locking may not help. But later we'll check log to avoid redo, although
### a redo does not hurt.
#
# Always keep lock file in a highly visisble directory so that after something goes wrong
# and the lock file is unable to be deleted in time, it will be sure to be deleted before
# reprocessing. That is, do not hide lock file in a temporary directory.
#
# Jun 26, 2018: File locking does not work on Cfncluster, but keep the code. Now twin B does not
# appear in the worklist, but it is looked for by twin A. So no race condition.
#
# Jul 12, 2019: No need to use locks now.

### Test for the presence of twins based on L1C filenames.
granbase_noAB=$(echo $granbase | sed 's/[AB]$//')
granbaseA=${granbase_noAB}A
granbaseB=${granbase_noAB}B
L1Ctwins=NO

dir=$(dirname $grandir)
if [ -d $dir/$granbaseA ] &&  [ -d $dir/$granbaseB ]
then
	L1Ctwins=YES
fi

if [ "$L1Ctwins" = YES ] 
then
	if  grep PPS10 $S2LOGDIR/$JOBNAME/*.[eo]* | grep $granbase | grep "$SUBDATE" -q
	then
		# An earlier process for the other twin has processed 
		# this twin as well, based on the log. DONE. 
		exit 0
	fi
fi

### Consolidate if necessary
# May 10, 2017: The above test for twins only determines whether there are twins 
# at the L1C level, for job synchronization. To see whether there are twins at the 
# surface reflectance level for consolidation, test the existence of rawS10 files
# since one of the twins could have failed during atmospheric correction.
#
tmpdir=$TDIR/S2/ppS10/consolidate
mkdir -p $tmpdir || exit 1

# Possible twin rawS10 from atmospheric correction  
rawS10A=$LOCAL_IO_DIR/S2/rawS10/$branchdir/HLS.S10.T${tileid}.${year}${doy}A.${HLSVER}.hdf
rawS10B=$LOCAL_IO_DIR/S2/rawS10/$branchdir/HLS.S10.T${tileid}.${year}${doy}B.${HLSVER}.hdf
outfile_con=$tmpdir/HLS.S10.T${tileid}.${year}${doy}.consolidated.hdf

SECONDS=0
if [ -f $rawS10A ] && [ -f $rawS10B ]           
then
	${CODE_BASE_DIR}/S2/ppS10/consolidate/consolidate $rawS10A $rawS10B $outfile_con
	if [ $? -ne 0 ]
	then
		echo "Error in consolidating for ${tileid}_${year}${doy}" >&2
		echo "Line $LINENO of ${BASH_SOURCE[0]}" >&2 
		now=$(date +%FT%H%M%S)
		echo "PPS10;$granbaseA;error;error_consolidate;$SUBDATE;$now" >&2
		echo "PPS10;$granbaseB;error;error_consolidate;$SUBDATE;$now" >&2
		exit 1
	fi
else
	# No twin granules at the surface reflectance level. Two possibilities:
	#  1) No twin granules at the TOA level (L1C)
	#  2) L1C twins exist, but one or both twins failed in atmospheric correction.
	#     Even if there is only one rawS10, it is not necessarily rawS10A: 
	#     twinA may have failed in atmospheric correction, but twinB survived.
	#
	# If L1C twins are present, one of them must have failed in atmospheric correction. As long as there is some
	# rawS10 data, move along.
	#
	# Rename to pretend as consolidated; NOT a symbolic link.
	#
	#
	if [ -f $rawS10A ]
	then
		# Dec 26, 2017: Even cp can fail due to i/o error! 
		# Log "DONE" at the end.
		cp $rawS10A $outfile_con      # why not "mv": keep input until successful finish.
		cp ${rawS10A}.hdr ${outfile_con}.hdr
	elif [ -f $rawS10B ] 
	then
		# Log "DONE" at the end.
		cp $rawS10B $outfile_con
		cp ${rawS10B}.hdr ${outfile_con}.hdr
	else
		# No rawS10
		now=$(date +%FT%H%M%S)
		if [ "$L1Ctwins" = NO ]
		then
			echo  "PPS10;$granbase;error;no_rawS10;$SUBDATE;$now"  >&2
		else
			echo  "PPS10;$granbaseA;error;no_rawS10;$SUBDATE;$now"  >&2
			echo  "PPS10;$granbaseB;error;no_rawS10;$SUBDATE;$now"  >&2
		fi

		exit 1
	fi
fi

#################### Co-registration if necessary

### Find the processing baseline of rawS10 from the S2 L1C xml.
#
# Note: not $grandir/*L1C.xml to account for earlier naming conventions. 12/1/2017
safexml=$(ls $grandir/*L1C*.xml) || exit 1
process_baseline=$(grep PROCESSING_BASELINE $safexml)

# Comment on May 10, 2017: In a few cases, one twin is before 2.04 and the other is after 2.04!!! 
# Not a big problem.
case "$process_baseline" in 
  *02.0[0-3]*) coregis=YES;;
            *) coregis=NO;;	# 2.04 and later
esac

if [ $coregis = NO ]
then	# No need to co-register. This is final S10.
	# Change time stamp.
	mv ${outfile_con}     ${outfile_s10};     touch ${outfile_s10}
	mv ${outfile_con}.hdr ${outfile_s10}.hdr; touch ${outfile_s10}.hdr
else
	### Resample based on AROP fitting
	# Double check on the availability of reference image; the same check is 
	# also performed in the AROP script.
	#
	# Apr 28, 2017: ESA changed the filenames.
	# refimg=$(ls $S2REFIMG_DIR/*T${tileid}_B08.jp2)
	refimg=$(ls $S2REFIMG_DIR/*${tileid}*_B08.bin)   # drop the leading T. Aug 15, 2017.
	nimg=$(echo $refimg | awk '{print NF}')           # Good check!
	if [ $nimg -ne 1 ]
	then
		echo "Wrong. There are $nimg reference image: $refimg " >&2
		echo "Line $LINENO of ${BASH_SOURCE[0]}" >&2 
		now=$(date +%FT%H%M%S)
		if [ "$L1Ctwins" = NO ]
		then
			echo "PPS10;$granbase;error;$nimg refimg;$SUBDATE;$now"  >&2
		else
			echo "PPS10;$granbaseA;error;$nimg refimg;$SUBDATE;$now"  >&2
			echo "PPS10;$granbaseB;error;$nimg refimg;$SUBDATE;$now"  >&2
		fi

		exit 1
	fi

	# Register with the reference image: AROP
	aropstdout=$( ${CODE_BASE_DIR}/S2/ppS10/AROPonS2/run.AROPonS2.sh $outfile_con)
	stat=$?
	if [ $stat -eq 1 ]
	then
		# Maybe warp image is too small or too cloudy; do not panic. Do not do anything.
		aropstdout=NONE
	elif [ $stat -eq 2 ]   # Some other error; must stop.
	then
		if [ "$L1Ctwins" = NO ]
		then
			echo "PPS10;$granbase;error;AROP;$SUBDATE;$now"  >&2
		else
			echo "PPS10;$granbaseA;error;AROP;$SUBDATE;$now"  >&2
			echo "PPS10;$granbaseB;error;AROP;$SUBDATE;$now"  >&2
		fi

		exit 1
	fi

	# Resample based on AROP fitting. $refimg is passed in only to be written in the metadata.
	${CODE_BASE_DIR}/S2/ppS10/resamp/resamp_s2 $outfile_con $refimg $aropstdout $outfile_s10
	if [ $? -ne 0 ]
	then
		echo "Error in resamp_s2 for $grandir" >&2
		echo "Line $LINENO of ${BASH_SOURCE[0]}" >&2 
		rm -f $outfile_s10 ${outfile_s10}.hdr
		now=$(date +%FT%H%M%S)	
		if [ "$L1Ctwins" = NO ]
		then
			echo "PPS10;$granbase;error;error in resamp_s2;$SUBDATE;$now"  >&2
		else
			echo "PPS10;$granbaseA;error;error in resamp_s2;$SUBDATE;$now"  >&2
			echo "PPS10;$granbaseB;error;error in resamp_s2;$SUBDATE;$now"  >&2
		fi

		exit 1
	fi
fi

### Update progress
hours=$(echo $SECONDS | awk '{printf("%.2f", $1/3600)}')
now=$(date +%FT%H%M%S)	
if [ "$L1Ctwins" = NO ]
then
	echo "PPS10;$granbase;DONE;$hours hours;$SUBDATE;$now"  >&2
else
	echo "PPS10;$granbaseA;DONE;$hours hours;$SUBDATE;$now" >&2
	echo "PPS10;$granbaseB;DONE;$hours hours;$SUBDATE;$now" >&2 
fi

exit 0
