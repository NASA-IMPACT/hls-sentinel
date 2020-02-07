/* Render all S2 bands (of S10) from their native resolutions (10,20,60m) to 30m.
 * Also changed the two masks from 10m to 30m.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "hls_projection.h"
#include "s2r.h"
#include "s2at30m.h"
#include "s2mapinfo.h"
#include "util.h"


/* #include "hls_hdfeos.h"
 * Nov 18, 2016: Decide not to make this intermediate product
 * in HDFEOS format, because additional metadata will be added
 * in downstream processing. If this product is made HDFEOS, 
 * the HDFEOS metadata will be buried by the susbsequent 
 * additional metadata, but it is better to make the HDFEOS
 * metadata the last item in the attributes.
 *
 *
 * Jul 29, 2019
 */

int copy_metadata(s2r_t *s2r, s2at30m_t *s2at30m);

int main(int argc, char *argv[])
{
	/* Command-line parameters */
	char fname_s2r[500];		/* input S10, 3 pixel-sizes */ 
	char fname_s2at30m[500];	/* output, S2 at 30m */

	s2r_t s2r;			/* Original S2 (S10) */
	s2at30m_t s2at30m;		/* 30m S2 (S30)*/

	char creationtime[50];
	int ret;

	if (argc != 3) {
		fprintf(stderr, "%s s2r  s2at30m \n", argv[0]);
		exit(1);
	}

	strcpy(fname_s2r,    argv[1]);
	strcpy(fname_s2at30m, argv[2]);

	/* Read the input */
	strcpy(s2r.fname, fname_s2r);
	ret = open_s2r(&s2r, DFACC_READ);
	if (ret != 0) {
		Error("Error in open_s2r()");
		exit(1);
	}

	/* Create an empty output of 30m S2 */
	strcpy(s2at30m.fname, fname_s2at30m);
	strcpy(s2at30m.zonehem, s2r.zonehem);
	s2at30m.ulx = s2r.ulx;
	s2at30m.uly = s2r.uly;
	s2at30m.nrow = s2r.nrow[0]/3;
	s2at30m.ncol = s2r.ncol[0]/3;
	ret = open_s2at30m(&s2at30m, DFACC_CREATE);
	if (ret != 0) {
		exit(1);
	}

	/* Resample to 30m */
	ret = resample_s2to30m(&s2r, &s2at30m);
	if (ret != 0)
		return(ret);

	/* Copy some attributes */
	/* Always check the return of a function. 11/27/2017 */
	ret = copy_metadata(&s2r, &s2at30m);
	if (ret != 0)
		return(ret);

	/* Update processing time */
	getcurrenttime(creationtime);
	SDsetattr(s2at30m.sd_id, HLSTIME, DFNT_CHAR8, strlen(creationtime), (VOIDP)creationtime);

	ret = close_s2r(&s2r);
	if (ret != 0)
		return(ret);
	ret = close_s2at30m(&s2at30m);
	if (ret != 0)
		return(ret);

	return 0;
}

/* Copy the input metadata, spatial and cloud cover from the S10 products.
 * Update the dimension and pixel sizes 
 */
int copy_metadata(s2r_t *s2r, s2at30m_t *s2at30m)
{
	int ret;
	ret = get_all_metadata(s2r);
	if (ret != 0)
		return(ret);

	/* Update for S30 */
	strcpy(s2r->nr, "3660");
	strcpy(s2r->nc, "3660");
	strcpy(s2r->spatial_resolution, "30");

	SDsetattr(s2at30m->sd_id, PRODUCT_URI, DFNT_CHAR8, strlen(s2r->uri), (VOIDP)s2r->uri);
	SDsetattr(s2at30m->sd_id, L1C_QUALITY, DFNT_CHAR8, strlen(s2r->quality), (VOIDP)s2r->quality);
	SDsetattr(s2at30m->sd_id, SPACECRAFT, DFNT_CHAR8, strlen(s2r->spacecraft), (VOIDP)s2r->spacecraft);
	SDsetattr(s2at30m->sd_id, TILE_ID, DFNT_CHAR8, strlen(s2r->tile_id), (VOIDP)s2r->tile_id);
	SDsetattr(s2at30m->sd_id, DATASTRIP_ID, DFNT_CHAR8, strlen(s2r->datastrip_id), (VOIDP)s2r->datastrip_id);
	SDsetattr(s2at30m->sd_id, PROCESSING_BASELINE, DFNT_CHAR8, strlen(s2r->baseline), (VOIDP)s2r->baseline);
	SDsetattr(s2at30m->sd_id, SENSING_TIME, DFNT_CHAR8, strlen(s2r->sensing_time), (VOIDP)s2r->sensing_time);
	SDsetattr(s2at30m->sd_id, L1PROCTIME, DFNT_CHAR8, strlen(s2r->l1proctime), (VOIDP)s2r->l1proctime);
	SDsetattr(s2at30m->sd_id, HORIZONTAL_CS_NAME, DFNT_CHAR8, strlen(s2r->cs_name), (VOIDP)s2r->cs_name);
	SDsetattr(s2at30m->sd_id, HORIZONTAL_CS_CODE, DFNT_CHAR8, strlen(s2r->cs_code), (VOIDP)s2r->cs_code);
	SDsetattr(s2at30m->sd_id, NROWS, DFNT_CHAR8, strlen(s2r->nr), (VOIDP)s2r->nr);
	SDsetattr(s2at30m->sd_id, NCOLS, DFNT_CHAR8, strlen(s2r->nc), (VOIDP)s2r->nc);
	SDsetattr(s2at30m->sd_id, SPATIAL_RESOLUTION, DFNT_CHAR8, strlen(s2r->spatial_resolution), (VOIDP)s2r->spatial_resolution);
	SDsetattr(s2at30m->sd_id, ULX, DFNT_FLOAT64, 1, (VOIDP)&(s2r->ululx));
	SDsetattr(s2at30m->sd_id, ULY, DFNT_FLOAT64, 1, (VOIDP)&(s2r->ululy));
	SDsetattr(s2at30m->sd_id, MSZ, DFNT_FLOAT64, 1, (VOIDP)&(s2r->msz));
	SDsetattr(s2at30m->sd_id, MSA, DFNT_FLOAT64, 1, (VOIDP)&(s2r->msa));
	SDsetattr(s2at30m->sd_id, MVZ, DFNT_FLOAT64, 1, (VOIDP)&(s2r->mvz));
	SDsetattr(s2at30m->sd_id, MVA, DFNT_FLOAT64, 1, (VOIDP)&(s2r->mva));

	SDsetattr(s2at30m->sd_id, SPCOVER, DFNT_INT16, 1, (VOIDP)&(s2r->spcover));
	SDsetattr(s2at30m->sd_id, CLCOVER, DFNT_INT16, 1, (VOIDP)&(s2r->clcover));
	SDsetattr(s2at30m->sd_id, ACCODE,  DFNT_CHAR8, strlen(s2r->accode), (VOIDP)s2r->accode); 

	/* AROP related */
	SDsetattr(s2at30m->sd_id, S_AROP_REFIMG, DFNT_CHAR8, strlen(s2r->refimg), (VOIDP)s2r->refimg);
	SDsetattr(s2at30m->sd_id, S_AROP_NCP,  DFNT_INT32, 1, (VOIDP)&s2r->ncp);
	SDsetattr(s2at30m->sd_id, S_AROP_RMSE, DFNT_FLOAT64, 1, (VOIDP)&s2r->rmse);
	SDsetattr(s2at30m->sd_id, S_AROP_XSHIFT, DFNT_FLOAT64, 1, (VOIDP)&s2r->xshift);
	SDsetattr(s2at30m->sd_id, S_AROP_YSHIFT, DFNT_FLOAT64, 1, (VOIDP)&s2r->yshift);

	return(0);
}
