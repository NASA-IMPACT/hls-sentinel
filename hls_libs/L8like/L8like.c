/* Adjust the 30m S2 reflectance to make it resemble L8 */ 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "s2at30m.h"
#include "util.h"
#include "hls_hdfeos.h"

/* Number of common bands between the two sensors. */
#define NCB 7

void write_spectral_slope_offset(s2at30m_t *s2o, double para[][2]);

int main(int argc, char *argv[])
{
	/* Command line parameters */
	char fname_para[LINELEN];
	char fname_out[LINELEN];  /* An copy of the NBAR, for spectral adjustment */

	int irow, icol, k;

	s2at30m_t s2o;
	FILE *fpara;

	double para[NCB][2];	/* slope and offset for 7 bands */
	int idx; 	/* Row index of an S2 band in the parameter array */
	char line[LINELEN];
	int i, ib;
	char creationtime[100];
	double tmpref;
	char message[MSGLEN];
	int ret;

	if (argc != 3) {
		fprintf(stderr, "%s para.txt out.hdf\n", argv[0]);
		exit(1);
	}

	strcpy(fname_para, argv[1]);
	strcpy(fname_out,  argv[2]);

	/* Read input S2 */
	strcpy(s2o.fname, fname_out);
	ret = open_s2at30m(&s2o, DFACC_WRITE);
	if (ret != 0)
		exit(1);

	/* Processing time */
	getcurrenttime(creationtime);
	SDsetattr(s2o.sd_id, HLSTIME, DFNT_CHAR8, strlen(creationtime), (VOIDP)creationtime);


	/* Read the bandpass adjustment parameters */
	if ((fpara = fopen(fname_para, "r")) == NULL) {
		sprintf(message, "Cannot read %s\n", fname_para);
		Error(message);
		exit(1);
	}
	fgets(line, sizeof(line), fpara);	/* skip header */
	for (i = 0; i < NCB; i++) {
		if (fgets(line, sizeof(line), fpara)) {	
			if (sscanf(line, "%lf%lf", &para[i][0], &para[i][1]) != 2) {
				sprintf(message, "Error in reading %s\n", fname_para);
				Error(message);
				exit(1);
			}
		}
		else {
			sprintf(message, "There are not enough lines in %s\n", fname_para);
			Error(message);
			exit(1);
		}
	}
	if (fgets(line, sizeof(line), fpara) != NULL) {	/* Should not have any more lines */
		sprintf(message, "There are extra lines data in %s", fname_para);
		Error(message);
		exit(1);
	}
	fclose(fpara);

	for (ib = 0; ib < S2NBAND; ib++) {
		/* Find the index in the parameter array for S2 */
		switch (ib) {
			case 0:  idx = 0; break;
			case 1:  idx = 1; break;
			case 2:  idx = 2; break;
			case 3:  idx = 3; break;
			case 8:  idx = 4; break; 	/* 8a */ /* Mar 23, 2016: The parameter file says band80 */
			case 11: idx = 5; break;		
			case 12: idx = 6; break;
			default: continue;
		}

		for (k = 0; k < s2o.nrow * s2o.ncol; k++) {
			if (s2o.ref[ib][k] != HLS_S2_FILLVAL) {	
				/* Ref scaling factor is 10000 */
				tmpref = s2o.ref[ib][k] * para[idx][0] + para[idx][1]*10000;
				s2o.ref[ib][k] =  asInt16(tmpref);
			}
		}
	}

	/* Write the spectral adjustment slope and offset */
	write_spectral_slope_offset(&s2o, para);
	if (close_s2at30m(&s2o) != 0) {
		Error("Error in close_s2at30m");
		return(1);
	}
	
	/* Make it hdfeos */
        sds_info_t all_sds[S2NBAND+2];	/* +2 masks */
        set_S30_sds_info(all_sds, S2NBAND+2, &s2o);
        ret = S30_PutSpaceDefHDF(&s2o, all_sds, S2NBAND+2);
        if (ret != 0) {
                Error("Error in HLS_PutSpaceDefHDF");
                exit(1);
        }

	return 0;	
}

void write_spectral_slope_offset(s2at30m_t *s2o, double para[][2])
{
	int band;
	char attrname[200], attrval[50];

	/* Mar 23, 2016: Mistakenly I had used DFNT_UCHAR8 */
	/* b1 - b4 */
	sprintf(attrname, "MSI band 01 bandpass adjustment slope and offset");
	sprintf(attrval,  "%lf, %lf", para[0][0], para[0][1]);
	SDsetattr(s2o->sd_id, attrname, DFNT_CHAR8, strlen(attrval), (VOIDP)attrval);

	sprintf(attrname, "MSI band 02 bandpass adjustment slope and offset");
	sprintf(attrval,  "%lf, %lf", para[1][0], para[1][1]);
	SDsetattr(s2o->sd_id, attrname, DFNT_CHAR8, strlen(attrval), (VOIDP)attrval);

	sprintf(attrname, "MSI band 03 bandpass adjustment slope and offset");
	sprintf(attrval,  "%lf, %lf", para[2][0], para[2][1]);
	SDsetattr(s2o->sd_id, attrname, DFNT_CHAR8, strlen(attrval), (VOIDP)attrval);

	sprintf(attrname, "MSI band 04 bandpass adjustment slope and offset");
	sprintf(attrval,  "%lf, %lf", para[3][0], para[3][1]);
	SDsetattr(s2o->sd_id, attrname, DFNT_CHAR8, strlen(attrval), (VOIDP)attrval);

	/* b8a */
	sprintf(attrname, "MSI band 8a bandpass adjustment slope and offset");
	sprintf(attrval,  "%lf, %lf", para[4][0], para[4][1]);
	SDsetattr(s2o->sd_id, attrname, DFNT_CHAR8, strlen(attrval), (VOIDP)attrval);

	/* b11-12 */
	sprintf(attrname, "MSI band 11 bandpass adjustment slope and offset");
	sprintf(attrval,  "%lf, %lf", para[5][0], para[5][1]);
	SDsetattr(s2o->sd_id, attrname, DFNT_CHAR8, strlen(attrval), (VOIDP)attrval);

	sprintf(attrname, "MSI band 12 bandpass adjustment slope and offset");
	sprintf(attrval,  "%lf, %lf", para[6][0], para[6][1]);
	SDsetattr(s2o->sd_id, attrname, DFNT_CHAR8, strlen(attrval), (VOIDP)attrval);
}
