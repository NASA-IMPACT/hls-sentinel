/******************************************************************************** 
 * Combine the twin granules fileA and fileB (i.e. two partial granules from the 
 * same sensor on the same day over the same tile due to datastrip transition) 
 * into one complete granule. 
 *
 * The input is atmospherically corrected reflectance.
 * 
 * Dec 21, 2016
 * Implementation note:
 *   I had tried to make output file a copy of fileA and use fileB to 
 *   update the output, but I got segmentation fault which I was unable to resolve. 
 *   So settle on the current clumsy design which creates a brand-new output file.
 *
 * Jul 8, 2019
 *   Use maximum NDVI composite for the overlap area because LaSRC sometimes
 *   does a poor job for the across-track edge of the image that has resulted 
 *   in stripes in v1.4, e.g. HLS.S10.T44QMD.2017014.v1.4.hdf. The max NDVI rule
 *   apparently selects better pixels, but there is no guarantee that terrible 
 *   atmospheric correction won't result in higher NDVI. (Finished on Jul 26, 2019)
 *
 * Apr 1, 2020: Note that the mean sun/view angles are from twin A only; later in 
 *   derive_s2nbar these angles will be recomputed and so set properly. 
 *
 * Mar 18, 2024:
 *	 The silly max NDVI rule in selecting between two twin granules is eliminated
 *	 because the leading/trailing edges of the twin granules have been trimmed where
 *	 some pixels are possibly corrupted. With trimming, data from either granule is good. 
 ********************************************************************************/

#include "s2r.h"
#include "util.h"
// #include "hls_hdfeos.h"	// Not needed. Sep 25, 2024

int copy_metadata_AB(s2r_t *s2rA, s2r_t *s2rB, s2r_t *s2rO);

/* Keep the pixel whose 10m row/col are given */
int copypix(s2r_t *from, int irow60m, int icol60m, s2r_t *to);

int main(int argc, char * argv[])
{
	/* Commandline parameters */
	char fnameA[LINELEN];
	char fnameB[LINELEN];
	char fnameO[LINELEN];	/* Consolidated, output */

	s2r_t s2rA, s2rB, s2rO;

	long k;
	int ib, psi, irow, icol, nrow, ncol;
	int irow60m, icol60m, nrow60m, ncol60m, k60m;
	int ubidx;	/* ultra-blue band index, it is 0 */
	int ret;
	char creationtime[50];
	char message[MSGLEN];

	if (argc != 4) {
		fprintf(stderr, "Usage: %s fileA fileB fout\n", argv[0]);
		exit(1);
	}

	strcpy(fnameA, argv[1]);
	strcpy(fnameB, argv[2]);
	strcpy(fnameO, argv[3]);

	/* Read the input A */
	strcpy(s2rA.fname, fnameA);
	ret = open_s2r(&s2rA, DFACC_READ);
	if (ret != 0) {
		Error("Error in open_s2r()");
		exit(1);
	}

	/* Read the input B */
	strcpy(s2rB.fname, fnameB);
	ret = open_s2r(&s2rB, DFACC_READ);
	if (ret != 0) {
		Error("Error in open_s2r()");
		exit(1);
	}

	/* Create the output */
	strcpy(s2rO.fname, fnameO);
	s2rO.nrow[0] = s2rA.nrow[0];
	s2rO.ncol[0] = s2rA.ncol[0];
	ret = open_s2r(&s2rO, DFACC_CREATE);
	if (ret != 0) {
		Error("Error in open_s2r()");
		exit(1);
	}

	/* Use a 60m band to guide the consolidation to make sure a 60m pixel
	 * and the nesting 10m and 20m pixels come from the same datastrip.. 
	 */
	ubidx = 0; 	
	nrow60m = s2rO.nrow[2];
	ncol60m = s2rO.ncol[2];
	for (irow60m = 0; irow60m < nrow60m; irow60m++) { 
		for (icol60m = 0; icol60m < ncol60m; icol60m++) { 
			k60m = irow60m * ncol60m + icol60m;
			/* Check on the first 60m band */
			if (s2rA.ref[ubidx][k60m] != HLS_REFL_FILLVAL ) 
				copypix(&s2rA, irow60m, icol60m, &s2rO);

			if (s2rB.ref[ubidx][k60m] != HLS_REFL_FILLVAL && s2rO.ref[ubidx][k60m] == HLS_REFL_FILLVAL) 
					copypix(&s2rB, irow60m, icol60m, &s2rO);
		}
	}

	/* Copy the metadata from twin granules. 
	 * Update HLS processing time, reset spatial and cloud coverages
	 */
	ret = copy_metadata_AB(&s2rA, &s2rB, &s2rO);
	if (ret != 0) {
		Error("Error in copy_metadata_AB");
		return(-1);
	}
	getcurrenttime(creationtime);
	SDsetattr(s2rO.sd_id, HLSTIME, DFNT_CHAR8, strlen(creationtime), (VOIDP)creationtime);
	setcoverage(&s2rO);

	/* A few variables needed to create the ENVI header */
	s2rO.ulx = s2rA.ulx;
	s2rO.uly = s2rA.uly;
	strcpy(s2rO.zonehem, s2rA.zonehem);

	/* Close */
	ret = close_s2r(&s2rO);
	if (ret != 0) {
		Error("Erro in close_s2r()");
		exit(1);
	}

	return(0);
}


/* Copy the input metadata from the twin granules A and B into into the consolidated 
 * granule, concatenating a few key metadata item that are different between A and B.
 * 
 * Dec 22, 2016. 
 * Jan 23, 2017.
 */
int copy_metadata_AB(s2r_t *s2rA, s2r_t *s2rB, s2r_t *s2rO)
{
	char attr[1000];
	/* SAFE name not available in Google data.  Nov 15, 2016.
	 * SDsetattr(s2at30m->sd_id, SAFE_NAME, DFNT_CHAR8, strlen(s2r->safe_name), (VOIDP)s2r->safe_name);
	 */
	if (get_all_metadata(s2rA) != 0) {
		Error("Error in copy_metadata_AB");
		return(-1);
	}
	if (get_all_metadata(s2rB) != 0) {
		Error("Error in copy_metadata_AB");
		return(-1);
	}

	/* URI */
	sprintf(attr, "%s + %s", s2rA->uri, s2rB->uri);	 /* Use '+' because ':' has been used in data quality string. */
	SDsetattr(s2rO->sd_id, PRODUCT_URI, DFNT_CHAR8, strlen(attr), (VOIDP)attr);

	/* Quality */
	sprintf(attr, "%s + %s", s2rA->quality, s2rB->quality);
	SDsetattr(s2rO->sd_id, L1C_QUALITY, DFNT_CHAR8, strlen(attr), (VOIDP)attr);

	/* Spacecraft, same */
	SDsetattr(s2rO->sd_id, SPACECRAFT,  DFNT_CHAR8, strlen(s2rA->spacecraft), (VOIDP)s2rA->spacecraft);

	/* tile ID. Should really be granuel ID */
	sprintf(attr, "%s + %s", s2rA->tile_id, s2rB->tile_id);
	SDsetattr(s2rO->sd_id, TILE_ID, DFNT_CHAR8, strlen(attr), (VOIDP)attr);

	/* DATASTRIP must be different */
	sprintf(attr, "%s + %s", s2rA->datastrip_id, s2rB->datastrip_id);
	SDsetattr(s2rO->sd_id, DATASTRIP_ID, DFNT_CHAR8, strlen(attr), (VOIDP)attr);

	/* BASELINE */
	sprintf(attr, "%s + %s", s2rA->baseline, s2rB->baseline);
	SDsetattr(s2rO->sd_id, PROCESSING_BASELINE, DFNT_CHAR8, strlen(attr), (VOIDP)attr);
	
	/* SENSING_TIME */
	sprintf(attr, "%s + %s", s2rA->sensing_time, s2rB->sensing_time);
	SDsetattr(s2rO->sd_id, SENSING_TIME, DFNT_CHAR8, strlen(attr), (VOIDP)attr);

	/* ACCODE */
	sprintf(attr, "%s + %s", s2rA->accode, s2rB->accode);
	SDsetattr(s2rO->sd_id, ACCODE, DFNT_CHAR8, strlen(attr), (VOIDP)attr);

	SDsetattr(s2rO->sd_id, L1PROCTIME, DFNT_CHAR8, strlen(s2rA->l1proctime), (VOIDP)s2rA->l1proctime);
	SDsetattr(s2rO->sd_id, HORIZONTAL_CS_NAME, DFNT_CHAR8, strlen(s2rA->cs_name), (VOIDP)s2rA->cs_name);
	SDsetattr(s2rO->sd_id, HORIZONTAL_CS_CODE, DFNT_CHAR8, strlen(s2rA->cs_code), (VOIDP)s2rA->cs_code);
	SDsetattr(s2rO->sd_id, NROWS, DFNT_CHAR8, strlen(s2rA->nr), (VOIDP)s2rA->nr);
	SDsetattr(s2rO->sd_id, NCOLS, DFNT_CHAR8, strlen(s2rA->nc), (VOIDP)s2rA->nc);
	SDsetattr(s2rO->sd_id, SPATIAL_RESOLUTION, DFNT_CHAR8, strlen(s2rA->spatial_resolution), (VOIDP)s2rA->spatial_resolution);
	SDsetattr(s2rO->sd_id, ULX, DFNT_FLOAT64, 1, (VOIDP)&(s2rA->ululx));
	SDsetattr(s2rO->sd_id, ULY, DFNT_FLOAT64, 1, (VOIDP)&(s2rA->ululy));
	SDsetattr(s2rO->sd_id, MSZ, DFNT_FLOAT64, 1, (VOIDP)&(s2rA->msz));
	SDsetattr(s2rO->sd_id, MSA, DFNT_FLOAT64, 1, (VOIDP)&(s2rA->msa));
	SDsetattr(s2rO->sd_id, MVZ, DFNT_FLOAT64, 1, (VOIDP)&(s2rA->mvz));
	SDsetattr(s2rO->sd_id, MVA, DFNT_FLOAT64, 1, (VOIDP)&(s2rA->mva));

	// coverage will be computed outside??? Jan 20, 2106.
	//SDsetattr(s2rO->sd_id, SPCOVER, DFNT_INT16, 1, (VOIDP)&(s2rA->spcover));
	//SDsetattr(s2rO->sd_id, CLCOVER, DFNT_INT16, 1, (VOIDP)&(s2rA->clcover));

	return(0);
}

int copypix(s2r_t *from, int irow60m, int icol60m, s2r_t *to)
{
	/* From the ultrablue band (60m) to find the nesting 10m/20m pixels. */ 
	int irow, icol, nrow, ncol, k;
	int rowbeg, rowend, colbeg, colend;
	int ib, psi;
	int bs;	/* number of pixels in one direction at the pixel size of interest
		 * within a 60m pixels. It is 1, 3, 6.
		 */

	for (ib = 0; ib < S2NBAND; ib++) {
		psi = get_pixsz_index(ib);
		if (psi == 0)
		       	bs = 6; 		/* 10m bands */
		else if (psi == 1) 
			bs = 3;			/* 20m bands */
		else if (psi == 2) 
			bs = 1;			/* 60m bands */
		else { 
			fprintf(stderr, "Wrong psi = %d\n", psi);
			exit(1);
		}

		ncol = from->ncol[psi];
		rowbeg = irow60m * bs;
		rowend = rowbeg + bs - 1;
		colbeg = icol60m * bs;
		colend = colbeg + bs - 1;
		
		for (irow = rowbeg; irow <= rowend; irow++) {
			for (icol = colbeg; icol <= colend; icol++) {
				k = irow * ncol + icol;
				to->ref[ib][k] = from->ref[ib][k];

				/* Set mask only once, when handling the first 10m band */
				if (ib == 1) {
					to->acmask[k] = from->acmask[k];
					to->fmask[k] = from->fmask[k];
				}
			}
		}
	}

	return 0;
}

