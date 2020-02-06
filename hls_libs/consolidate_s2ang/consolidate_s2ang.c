/* Consolidate the angles from the twin granules into one granule.
 * Dec 27, 2016.
 */

#include <stdio.h>
#include <stdlib.h>
#include "hls_commondef.h"
#include "s2ang.h"
#include "util.h"

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
	ret = open_s2ang(&s2angC, DFACC_CREATE);
	if (ret != 0) {
		Error("Error in open_s2ang");
		exit(1);
	}

	/* consolidate */
	for (irow = 0; irow < s2angC.nrow; irow++) {
		for (icol = 0; icol < s2angC.ncol; icol++) {
			k = irow * s2angC.ncol + icol;
			for (ib = 0; ib < S2NBAND; ib++) {
				if (s2angA.vz[ib][k] != ANGFILL) {
					s2angC.vz[ib][k] = s2angA.vz[ib][k]; 
					s2angC.va[ib][k] = s2angA.va[ib][k]; 
				}
				if (s2angB.vz[ib][k] != ANGFILL) {
					s2angC.vz[ib][k] = s2angB.vz[ib][k]; 
					s2angC.va[ib][k] = s2angB.va[ib][k]; 
				}
			}

			/* Solar angles are actually identical in twin granules */
			if (s2angA.sz[k] != ANGFILL) {
				s2angC.sz[k] = s2angA.sz[k];
				s2angC.sa[k] = s2angA.sa[k];
			}
			if (s2angB.sz[k] != ANGFILL) {
				s2angC.sz[k] = s2angB.sz[k];
				s2angC.sa[k] = s2angB.sa[k];
			}
		}
	}

	/* Angle is considered available if it is savailable for both of the twin granules */
	for (ib = 0; ib < S2NBAND; ib++) {
		s2angC.angleavail[ib] = (s2angA.angleavail[ib] == 1 && s2angB.angleavail[ib] == 1);
		SDsetattr(s2angC.sd_id, ANGLEAVAIL, DFNT_UINT8, S2NBAND, s2angC.angleavail);
	}

	ret = close_s2ang(&s2angC);
	if (ret != 0) {
		Error("Error in close_s2ang");
		exit(1);
	}

	return 0;
}
