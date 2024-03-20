/******************************************************************************** 
 * 1) The images of different spectral bands do not start from and end at the 
 * same ground location. Trim the image edge so that in the output a location 
 * either has data in all spectral bands or has no data in any spectral band. 
 * The trimming has effect in the interior of the images too. If atmospheric
 * correction makes the data invalid at a pixel in one spectral band, the pixel
 * will have no data in all spectral bands after trimming.
 * 
 * 2) The leading or trailing edge of a twin granule can have corrupted data that
 * ESA does not flag. So trim the edge in all spectral bands by a length of n
 * 60-m pixels. N = 2 for now.
 * The side effect is that the leading/trailing edge of the data take will be 
 * trimmed too, but the data loss there has no significance.
 *
 * First implemented on Apr 8, 2020, and added trailing/leading edge trimming 
 * on Mar 4, 2024
 *
 ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "hls_projection.h"
#include "s2r.h"
#include "util.h"
#include "hls_hdfeos.h"

int main(int argc, char *argv[])
{
	/* The only command-line parameter */
	char fname_s2rin[500];		/* Open for update */

	s2r_t s2rin; 			
	int irow60m, icol60m, nrow60m, ncol60m, k60m;
	int irow, icol, nrow, ncol, k;
	int rowbeg, rowend, colbeg, colend;
	char missing;
	int bs;

	/*** A few variables used for leading/trailing edge trimming */
	char isFill;
	/* Number of pixels to be trimmed in the 60-m bands at the leading/trailing edge*/
	int N60m = 2;	
	int startRow, endRow; /* Defining the rows to be trimmed */

	char creationtime[100];
	int ret;

	if (argc != 2) {
		fprintf(stderr, "%s s2rin \n", argv[0]);
		exit(1);
	}

	strcpy(fname_s2rin, argv[1]);

	/* Open the input for read*/
	strcpy(s2rin.fname, fname_s2rin);
	ret = open_s2r(&s2rin, DFACC_WRITE);
	if (ret != 0) {
		Error("Error in open_s2r");
		exit(1);
	}

	/* Use the 60m aerosol band to guide the trimming. If there is nodata value in any 
	 * spectral band for the 60m x 60m pixel, the entire 60m x 60m area in all spectral 
	 * bands area will be filled with nodata.
	 */
	nrow60m = s2rin.nrow[2];
	ncol60m = s2rin.ncol[2];

	/* Leading/trailing edge trimming in the coastal/aerosol band by N pixels, and this
	 * will have an effect on the subsquent trimming in all bands. Added March 2024.
	 */
	for (icol60m = 0; icol60m < ncol60m; icol60m++) {

		/*** First check if the top of each column has fill value that indicates the edge */
		isFill = 0;	/* Fillvalue or not */
		startRow = -1;	/* Looking from the top down. The row that starts to have non-fill 
						 * value and the previous row has fill value */
		for (irow60m = 0; irow60m < nrow60m; irow60m++) {  
			k60m = irow60m * ncol60m + icol60m;
			if (s2rin.ref[0][k60m] == HLS_REFL_FILLVAL) {
				isFill = 1;
				continue;
			}
			else {
				if (isFill == 1)	/* The row above is fill; edge detected */
					startRow = irow60m;

				break;	/* Break whenever nonfill is found */
			}
		}

		/* Set pixels in this column from startRow to endRow to fill value */
		if (startRow != -1) {
			endRow = startRow + N60m-1;
			if (endRow >= nrow60m)
				endRow = nrow60m-1;	/* the last row of the image in the 60m band*/

			for (irow60m = startRow; irow60m <= endRow; irow60m++) {  /* including endRow */
				k60m = irow60m * ncol60m + icol60m;
				s2rin.ref[0][k60m] = HLS_REFL_FILLVAL;
			}
		}

		/*** Then check if the bottom of each column has fill value */
		isFill = 0;	/* Fillvalue or not */
		endRow = -1;	/* Looking from the bottom up. The row that starts to have non-fill 
						 * value and the previous row is fill value */
		for (irow60m = nrow60m-1; irow60m >= 0; irow60m--) {  
			k60m = irow60m * ncol60m + icol60m;
			if (s2rin.ref[0][k60m] == HLS_REFL_FILLVAL) {
				isFill = 1;
				continue;
			}
			else {
				if (isFill == 1)	/* The row below is fill; edge detected */
					endRow = irow60m;

				break;	/* Break whenever nonfill is found */
			}
		}

		/* Set pixels in this column from startRow to endRow to fill value */
		if (endRow != -1) {
			startRow = endRow - (N60m-1);
			if (startRow <= 0)
				startRow = 0;

			for (irow60m = startRow; irow60m <= endRow; irow60m++) {  /* including endRow */
				k60m = irow60m * ncol60m + icol60m;
				s2rin.ref[0][k60m] = HLS_REFL_FILLVAL;
			}
		}
	}	/* End of leading/trailing edge trimming */



	for (irow60m = 0; irow60m < nrow60m; irow60m++) {
		for (icol60m = 0; icol60m < ncol60m; icol60m++) {
			k60m = irow60m * ncol60m + icol60m;

			/*** First pass to detect missing data ***/
			missing = 0;

			/* 60m bands */
			if (s2rin.ref[0][k60m]  == HLS_REFL_FILLVAL ||
			    s2rin.ref[9][k60m]  == HLS_REFL_FILLVAL ||
			    s2rin.ref[10][k60m] == HLS_REFL_FILLVAL) 
			       missing = 1;

			/* 20m pixels */
			if (!missing) {
				bs = 3; 
				ncol = s2rin.ncol[1]; 	
				rowbeg = irow60m * bs;
				rowend = rowbeg + bs - 1;
				colbeg = icol60m * bs;
				colend = colbeg + bs - 1;

				for (irow = rowbeg; irow <= rowend; irow++) {
					for (icol = colbeg; icol <= colend; icol++) {
						k = irow * ncol + icol;
						if (s2rin.ref[4][k]  == HLS_REFL_FILLVAL ||
						    s2rin.ref[5][k]  == HLS_REFL_FILLVAL ||
						    s2rin.ref[6][k]  == HLS_REFL_FILLVAL ||
						    s2rin.ref[8][k]  == HLS_REFL_FILLVAL ||
						    s2rin.ref[11][k] == HLS_REFL_FILLVAL ||
						    s2rin.ref[12][k] == HLS_REFL_FILLVAL)
							missing = 1; 
					}
				}
			}

			/* 10m pixels */
			if (!missing) {
				bs = 6; 
				ncol = s2rin.ncol[0]; 	
				rowbeg = irow60m * bs;
				rowend = rowbeg + bs - 1;
				colbeg = icol60m * bs;
				colend = colbeg + bs - 1;

				for (irow = rowbeg; irow <= rowend; irow++) {
					for (icol = colbeg; icol <= colend; icol++) {
						k = irow * ncol + icol;
						if (s2rin.ref[1][k] == HLS_REFL_FILLVAL ||
						    s2rin.ref[2][k] == HLS_REFL_FILLVAL ||
						    s2rin.ref[3][k] == HLS_REFL_FILLVAL ||
						    s2rin.ref[7][k] == HLS_REFL_FILLVAL) 
							missing = 1; 
					}
				}
			}

			/*** Second pass to set to missing***/
			if (missing) {
				/* 60m bands */
				s2rin.ref[0][k60m]  = HLS_REFL_FILLVAL;
				s2rin.ref[9][k60m]  = HLS_REFL_FILLVAL;
				s2rin.ref[10][k60m] = HLS_REFL_FILLVAL; 

				/* 20m pixels */
				bs = 3; 
				ncol = s2rin.ncol[1]; 	
				rowbeg = irow60m * bs;
				rowend = rowbeg + bs - 1;
				colbeg = icol60m * bs;
				colend = colbeg + bs - 1;
	
				for (irow = rowbeg; irow <= rowend; irow++) {
					for (icol = colbeg; icol <= colend; icol++) {
						k = irow * ncol + icol;
						s2rin.ref[4][k]  = HLS_REFL_FILLVAL;
						s2rin.ref[5][k]  = HLS_REFL_FILLVAL;
						s2rin.ref[6][k]  = HLS_REFL_FILLVAL;
						s2rin.ref[8][k]  = HLS_REFL_FILLVAL;
						s2rin.ref[11][k] = HLS_REFL_FILLVAL;
						s2rin.ref[12][k] = HLS_REFL_FILLVAL;
					}
				}
	
				/* 10m pixels */
				bs = 6; 
				ncol = s2rin.ncol[0]; 	
				rowbeg = irow60m * bs;
				rowend = rowbeg + bs - 1;
				colbeg = icol60m * bs;
				colend = colbeg + bs - 1;
	
				for (irow = rowbeg; irow <= rowend; irow++) {
					for (icol = colbeg; icol <= colend; icol++) {
						k = irow * ncol + icol;
						s2rin.ref[1][k] = HLS_REFL_FILLVAL;
						s2rin.ref[2][k] = HLS_REFL_FILLVAL;
						s2rin.ref[3][k] = HLS_REFL_FILLVAL;
						s2rin.ref[7][k] = HLS_REFL_FILLVAL;
						
						/* ACmask and Fmask at 10m*/
						s2rin.acmask[k] = HLS_MASK_FILLVAL;
						s2rin.fmask[k]  = HLS_MASK_FILLVAL;
					}
				}
			}
		}
	}

	/* Processing time */
	getcurrenttime(creationtime);
	SDsetattr(s2rin.sd_id, HLSTIME, DFNT_CHAR8, strlen(creationtime), (VOIDP)creationtime);

	/* spatial and cloud. Cloud coverage relies on QA SDS */
	setcoverage(&s2rin);

	if (close_s2r(&s2rin) != 0) {
		Error("Error in closing output");
		exit(1);
	}

	/* Make it hdfeos */
 	sds_info_t all_sds[S2NBAND+2];
	set_S10_sds_info(all_sds, S2NBAND+2, &s2rin);

	ret = S10_PutSpaceDefHDF(&s2rin, all_sds, S2NBAND+2); 
	if (ret != 0) {
		Error("Error in HLS_PutSpaceDefHDF");
		exit(1);
	}

	return 0;
}
