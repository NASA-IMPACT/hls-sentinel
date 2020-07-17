/*********************************************************************************
  The LaSRCS2 code separates the output into two HDF files, with all SDS at 10m.
  This code (1) combines the two files into one, 
	    (2) aggregates from 10m for the 20m and 60m bands. 
   		CLOUD SDS is kept at 10m.
	    (3) reflectance fillval changed from -100 to HLS_REFL_FILLVAL
		Note: June 10, 2020
			The Fortran output uses -100 as the fillval, but the C code uses -9999.
		CLOUD fillval changed from -24 to 255. (in s2r.c)
	    (4) add some attributes from the XML files.
	    (5) July 17, 2020
	        a. To change LSRD LaSRC 3.02 uint16 result to int16.
		   Although HLS data type is int16, the HLS I/O code reads the bits of uint16 correctly
		   and these bits can be cast back to uint16.
		b. LSRD LaSRC 3.02 uses (refl + 0.2) * (1/0.0000275); change back to refl * 10000

	File one:
	short band01(fakeDim0, fakeDim1) ;
	short blue(fakeDim2, fakeDim3) ;
	short green(fakeDim4, fakeDim5) ;
	short red(fakeDim6, fakeDim7) ;
	short band05(fakeDim8, fakeDim9) ;
	short band06(fakeDim10, fakeDim11) ;
	short band07(fakeDim12, fakeDim13) ;
	short band08(fakeDim14, fakeDim15) ;

	File two:
	short band8a(fakeDim0, fakeDim1) ;
	short band09(fakeDim2, fakeDim3) ;
	short band10(fakeDim4, fakeDim5) ;
	short band11(fakeDim6, fakeDim7) ;
	short band12(fakeDim8, fakeDim9) ;
	byte CLOUD(fakeDim10, fakeDim11) ;

	The following three SDS are no longer available:  Jun 23, 2017. 
	But David Roy's group examined the AOT retrieval; they must have been retained in 
	a customized code for them.  Jun 26, 2019.
	short aot550nm(fakeDim10, fakeDim11) ;
	short residual(fakeDim12, fakeDim13) ;
	short tratiob1(fakeDim14, fakeDim15) ;
*********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "hls_projection.h"
#include "s2r.h"
#include "util.h"

/* Read from two hdf files, and read map projection info from the granule XML. */
int read_twohdf(s2r_t *s2r, char *fname1, char *fname2, char* fname_granulexml);

int main(int argc, char *argv[])
{
	char fname_part1[500];
	char fname_part2[500];
	char fname_safexml[500];  	/* XML for the overall SAFE */
	char fname_granulexml[500];
	char accodename[100];		/* Atmospheric correction code name */
	char fname_out[500];

	s2r_t s2in;
	s2r_t s2out;
	int ib, boxsize;
	long kin, kout;
	int rowstart, rowend, colstart, colend;
	int irow, icol, nrow, ncol;
	int ir, ic;
	double sum;
	int n, k, ntot;
	int ret;
	char creationtime[50];
	unsigned short us;	/* To copy the int16 bits into this. Jul 17, 2020 */

	if (argc != 7) {
		fprintf(stderr, "Usage: %s part1 part2 safexml granulexml accodename out\n", argv[0]);
		exit(1);
	}

	strcpy(fname_part1, argv[1]);
	strcpy(fname_part2, argv[2]);
	strcpy(fname_safexml,    argv[3]);
	strcpy(fname_granulexml, argv[4]);
	strcpy(accodename,       argv[5]);
	strcpy(fname_out,        argv[6]);

	/* Read input SDS and map info */
	ret = read_twohdf(&s2in, fname_part1, fname_part2, fname_granulexml);
	if (ret != 0) {
		Error("Error in reading two hdf");
		exit(1);
	} 

	/* Create output */
	strcpy(s2out.fname, fname_out);
	s2out.nrow[0] = s2in.nrow[0];
	s2out.ncol[0] = s2in.ncol[0];
	s2out.ulx = s2in.ulx;
	s2out.uly = s2in.uly;
	strcpy(s2out.zonehem, s2in.zonehem);
	strcpy(s2out.mask_unavailable, MASK_UNAVAILABLE);	/* No two mask SDS to create yet */
	strcpy(s2out.ac_cloud_available, AC_CLOUD_AVAILABLE);	/* There is a CLOUD SDS to copy over*/
	ret = open_s2r(&s2out, DFACC_CREATE);
	if (ret != 0) {
		exit(1);
	}

	/* Get some metadata from the two XML and write to output */
	if (setinputmeta(&s2out, fname_safexml, fname_granulexml, accodename) != 0) {
		Error("Error in setinputmeta");
		exit(1);
	}

	/* Processing time */
	getcurrenttime(creationtime);
	SDsetattr(s2out.sd_id, HLSTIME, DFNT_CHAR8, strlen(creationtime), (VOIDP)creationtime);

	/* Resample the 20m and 60 bands to their original resolutions; for 10m bands, a simple copy */
	/* Can't use the dup_s2 function because the input has no QA bands */
	for (ib = 0; ib < S2NBAND; ib++) {
		switch (ib) {
			case  0: boxsize = 6; break;
			case  1: boxsize = 1; break;
			case  2: boxsize = 1; break;
			case  3: boxsize = 1; break;
			case  4: boxsize = 2; break;
			case  5: boxsize = 2; break;
			case  6: boxsize = 2; break;
			case  7: boxsize = 1; break;
			case  8: boxsize = 2; break;
			case  9: boxsize = 6; break;
			case 10: boxsize = 6; break;
			case 11: boxsize = 2; break;
			case 12: boxsize = 2; break;
		} 
		
		/* Output dimension for a 10m, 20m, or 60m band */
		nrow = s2in.nrow[0]/boxsize;
		ncol = nrow;

		ntot = boxsize * boxsize;
		for (irow = 0; irow < nrow; irow++) {
			for (icol = 0; icol < ncol; icol++) {
				/* pixel indices at 10m */
				rowstart = irow * boxsize;	
				rowend = rowstart + boxsize -1;
				colstart = icol * boxsize;
				colend = colstart + boxsize -1;

				sum = 0.0;
				n = 0;
				for (ir = rowstart; ir <= rowend; ir++) {
					for (ic = colstart; ic <= colend; ic++) {
						kin = ir * s2in.ncol[0] + ic;
						// July 19, 2020: 0 is EROS nodata value.
						//if (s2in.ref[ib][kin] != HLS_REFL_FILLVAL) {
					        //	sum += s2in.ref[ib][kin];
						// HLS code reads LSRD uint16 as int16, but
						// the int16 and uint16 bits are the same. Recast to uint16. 
						memcpy(&us, &s2in.ref[ib][kin], 2);
						if (us > 0 ) {
							sum += us;
							n++;
						}
					}
				}

				/* Make sure there is no fill value in the box.  Jun 26, 2019.  */	
				if (n == ntot) {
					kout = irow * ncol + icol;
					sum = (sum * 0.0000275 - 0.2) * 10000;
					s2out.ref[ib][kout] = asInt16(sum / n);
				}
			}
		}
	}

	/* CLOUD SDS. Direct copy. 10m in, 10m out */
	for (irow = 0; irow < s2in.nrow[0]; irow++) {
		for (icol = 0; icol < s2in.ncol[0]; icol++) {
			k = irow * s2in.ncol[0] + icol;
			s2out.accloud[k] = s2in.accloud[k];
		}
	}

	close_s2r(&s2out);

	return(0);
}


/* The LaSRCS2 output is in two hdf files. Read both. */
int read_twohdf(s2r_t *s2r, char *fname1, char *fname2, char *fname_granulexml)
{
	char fname[500];
	char sds_name[500];     
	int32 sds_index;
	int32 sd_id, sds_id;
	int32 nattr, attr_index;
	//char attr_name[200];
	int32 dimsizes[2];
	int32 rank, data_type, n_attrs;
	int32 start[2], edge[2];

	int ib;
	char message[MSGLEN];

	for (ib = 0; ib < S2NBAND; ib++) 
		s2r->ref[ib] = NULL;
	s2r->accloud = NULL; 	/* CLOUD SDS is new */

	for (ib = 0; ib < S2NBAND; ib++) { 
		if (ib == 0 || ib == 8) { 
			if (ib == 0)
				strcpy(fname, fname1);
			else
				strcpy(fname, fname2);
				
			if ((sd_id = SDstart(fname, DFACC_READ)) == FAIL) {
				sprintf(message, "Cannot open %s", fname);
				Error(message);
				return(ERR_READ);
			}
		}
		
		strcpy(sds_name, VermoteS2sdsname[ib]);
		if ((sds_index = SDnametoindex(sd_id, sds_name)) == FAIL) {
			sprintf(message, "Didn't find the SDS %s in %s", sds_name, fname);
			Error(message);
			return(ERR_READ);
		}

		sds_id = SDselect(sd_id, sds_index);
		if (SDgetinfo(sds_id, sds_name, &rank, dimsizes, &data_type, &nattr) == FAIL) {
			Error("Error in SDgetinfo");
			return(ERR_READ);
		}

		/* All bands are at 10m resolution actually */
		s2r->nrow[0] = dimsizes[0];
		s2r->ncol[0] = dimsizes[1];

		/* Allocate memory for read access */ 
		if ((s2r->ref[ib] = (int16*)calloc(dimsizes[0] * dimsizes[1], sizeof(int16))) == NULL) {
			sprintf(message, "Cannot allocate memory. nrow, ncol = %d, %d\n", dimsizes[0], dimsizes[1]);
			Error(message);
			return(1);
		}

		start[0] = 0; edge[0] = dimsizes[0];
		start[1] = 0; edge[1] = dimsizes[1];
		if (SDreaddata(sds_id, start, NULL, edge, s2r->ref[ib]) == FAIL) {
			sprintf(message, "Error reading sds %s in %s", sds_name, fname);
			Error(message);
			return(ERR_READ);
		}
		SDendaccess(sds_id);
	}

	/* Read the CLOUD sds from the second file. sd_id now refers to the second file */
	strcpy(sds_name, AC_CLOUD_NAME);
	if ((sds_index = SDnametoindex(sd_id, sds_name)) == FAIL) {
		sprintf(message, "Didn't find the SDS %s in %s", sds_name, fname);
		Error(message);
		return(ERR_READ);
	}
	sds_id = SDselect(sd_id, sds_index);
	if ((s2r->accloud = (uint8*)calloc(dimsizes[0] * dimsizes[1], sizeof(uint8))) == NULL) {
		sprintf(message, "Cannot allocate memory. nrow, ncol = %d, %d\n", dimsizes[0], dimsizes[1]);
		Error(message);
		return(1);
	}
	if (SDreaddata(sds_id, start, NULL, edge, s2r->accloud) == FAIL) {
		sprintf(message, "Error reading sds %s in %s", sds_name, fname);
		Error(message);
		return(ERR_READ);
	}
	SDendaccess(sds_id);


	/******** Read ULX, ULY, zonehem from xml */
	char line[500];
	char *chpos1, *chpos2, cs_name[50];
	int len;
	FILE *fxml;
	char found_csname, found_ulx, found_uly;

	if ((fxml = fopen(fname_granulexml, "r")) == NULL) {
		sprintf(message, "Cannot open %s", fname_granulexml);
		Error(message);
		return(1);
	}
	found_csname = found_ulx = found_uly = 0;
	while (fgets(line, sizeof(line), fxml)) {
		if (strstr(line, HORIZONTAL_CS_NAME)) {
			/* Jun 9, 2016: This items contains 20 char exactly. With '\0'
			 * the C char array should have minimum length 21, but I declared
			 * 20. A bug took a few hours to find. 
			 */
			chpos1 = strchr(line, '>');
			chpos2 = strchr(chpos1, '<');
			len = chpos2 - (chpos1+1);
			strncpy(cs_name, chpos1+1, len);
			cs_name[len] = '\0';
			chpos1 = strrchr(cs_name, ' ');
			strcpy(s2r->zonehem, chpos1 + 1);

			found_csname = 1;
		} 
		else if (strstr(line, ULX)) {
			chpos1 = strchr(line, '>');
			s2r->ulx = atof(chpos1+1);
			found_ulx = 1;
		}
		else if (strstr(line, ULY)) {
			chpos1 = strchr(line, '>');
			s2r->uly = atof(chpos1+1);
			found_uly = 1;
		}
	}
	if ( ! found_csname) {
		Error("HORIZONTAL_CS_NAME not found");
		exit(1);
	}
	if ( ! found_ulx) {
		Error("ULX not found");
		exit(1);
	}
	if ( ! found_uly) {
		Error("ULY not found");
		exit(1);
	}

	/* Added on Oct 3, 2018: To GCTP convention */
	if (strstr(s2r->zonehem, "S")) {
		s2r->uly -= pow(10,7);
	}
	return(0);
}
