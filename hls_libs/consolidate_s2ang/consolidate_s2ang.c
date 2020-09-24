/* Consolidate the angles from the twin granules into one granule.
 * Dec 27, 2016.
 * Sep 23, 2020: 4 bands and make it HDF-EOS.
 */

#include <stdio.h>
#include <stdlib.h>
#include "hls_commondef.h"
#include "s2ang.h"
#include "util.h"
#include "hls_hdfeos.h"

int main(int argc, char *argv[])
{
	char fname_angA[LINELEN];	/* angle in twin granule A */
	char fname_angB[LINELEN];	/* angle in twin granule B */
	char fname_angC[LINELEN]; 	/* angle in consolidated output */

	s2ang_t s2angA, s2angB, s2angC;

	int irow, icol, k, ib;
	char message[MSGLEN];

	int ret;

	if (argc != 4) {
		fprintf(stderr, "%s angA angB angC\n", argv[0]);
		exit(1);
	}

	strcpy(fname_angA, argv[1]);
	strcpy(fname_angB, argv[2]);
	strcpy(fname_angC, argv[3]);

	/* Read A */
	strcpy(s2angA.fname, fname_angA);
	ret = open_s2ang(&s2angA, DFACC_READ);
	if (ret != 0) {
		Error("Error in open_s2ang");
		exit(1);
	}

	/* Read B */
	strcpy(s2angB.fname, fname_angB);
	ret = open_s2ang(&s2angB, DFACC_READ);
	if (ret != 0) {
		Error("Error in open_s2ang");
		exit(1);
	}

	/* Create C */
	strcpy(s2angC.fname, fname_angC);
	s2angC.nrow = s2angA.nrow;
	s2angC.ncol = s2angA.ncol;
	s2angC.ulx = s2angA.ulx;
	s2angC.uly = s2angA.uly;
	strcpy(s2angC.zonehem, s2angA.zonehem);
	ret = open_s2ang(&s2angC, DFACC_CREATE);
	if (ret != 0) {
		Error("Error in open_s2ang");
		exit(1);
	}

	/* consolidate */
	for (ib = 0; ib < NANG; ib++) {
		for (irow = 0; irow < s2angC.nrow; irow++) {
			for (icol = 0; icol < s2angC.ncol; icol++) {
				k = irow * s2angC.ncol + icol;
				if (s2angA.ang[ib][k] != ANGFILL) {
					s2angC.ang[ib][k] = s2angA.ang[ib][k]; 
					s2angC.ang[ib][k] = s2angA.ang[ib][k]; 
				}
				if (s2angB.ang[ib][k] != ANGFILL) {
					s2angC.ang[ib][k] = s2angB.ang[ib][k]; 
					s2angC.ang[ib][k] = s2angB.ang[ib][k]; 
				}
			}
		}
	}

	ret = close_s2ang(&s2angC);
	if (ret != 0) {
		Error("Error in close_s2ang");
		exit(1);
	}

	/* Make it HDF-EOS */
	sds_info_t all_sds[NANG];
        set_S2ang_sds_info(all_sds, NANG, &s2angC);
        ret = S2ang_PutSpaceDefHDF(&s2angC, all_sds, NANG);
        if (ret != 0) {
                Error("Error in HLS_PutSpaceDefHDF");
                exit(1);
        }


	return 0;
}
