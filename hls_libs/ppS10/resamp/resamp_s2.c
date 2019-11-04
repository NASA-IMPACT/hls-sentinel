/* Based on AROP fitting, eesample the 10m/20m/60m S10 surface reflectance and the 
 * 10m ACmask and Fmask.  
 * 
 *
 * Dec 23, 2016
 *
 * Nov 13, 2017.   
 * For reflectance, use cubic convolution; if any value in the 4x4 convolution 
 * window is fill, the output reflectance is fill. But for QA resample, a 4x4 
 * window may be too big since the weights used in cubic convolution can't be 
 * straightforwardly applied and equal weights for all 16 pixels would be unfair.
 * A compromise is used: only the inner 2x2 window inside the 4x4 convolution 
 * window is considered and if any bit or bit group in the 2x2 window is set,
 * the output QA is set. For the 2-bit aerosol level, bigger values take
 * precedence over lower values.
 *
 * This resampling decision is purely arbitrary: first, the 4x4 kernel covers 
 * different ground area for different pixel sizes; second, QA resampling
 * window is further different from those used for reflectance. And it is strict.
 * 
 * Jul 9, 2019
 *   Two QA SDS: ACmask and Fmask.  If ESA reprocesses its archive to a better
 *   geolocation accuracy, this function will not be invoked at all.
 */

#include "s2r.h"
#include "util.h"
#include "hls_commondef.h"
#include "read_arop_output.h"
#include "cubic_conv.h"
#include "hls_hdfeos.h"


int copy_metadata(s2r_t *s2r, s2r_t *s2rout);

int set_arop_metadata(	s2r_t *s2rout,
			char *refimgname,
			int ncp,
			double rmse,
			double xshift,
			double yshift);

int main(int argc, char *argv[])
{
	char fname_s10in[LINELEN];
	char fname_ref[LINELEN];	/* reference image name */
	char fname_fit[LINELEN];	/* AROP fitting */
	char fname_s10out[LINELEN];

	s2r_t s2rin, s2rout;

	/* AROP */
	double coeff[2][3]; /* AROP fitted 1st order polynomial coeff */
	int ncp;	    /* Number of control point in AROP */
	double rmse; 	   
	double xshift, yshift; /* The average amount of  shift based on AROP */
	char arop_valid;	/* Whether valid AROP fitting is available */	

	char creationtime[50];
	int ret;
	char message[MSGLEN];

	if (argc != 5) {
		fprintf(stderr, "Usage: %s S10in refimg aropfit S10out\n", argv[0]);
		exit(1);
	}

	strcpy(fname_s10in,  argv[1]);
	strcpy(fname_ref,    argv[2]);
	strcpy(fname_fit,    argv[3]);
	strcpy(fname_s10out, argv[4]);

	/* Read input S2 */
	strcpy(s2rin.fname, fname_s10in);
	ret = open_s2r(&s2rin, DFACC_READ);
	if (ret != 0 ) {
		Error("Error in open_s2r()");	
		exit(1);
	}

	/* Create output */
	strcpy(s2rout.fname, fname_s10out);
	s2rout.nrow[0] = s2rin.nrow[0];
	s2rout.ncol[0] = s2rin.ncol[0];
	ret = open_s2r(&s2rout, DFACC_CREATE);
	if (ret != 0 ) {
		Error("Error in open_s2r()");	
		exit(1);
	}

	/* Read AROP fitting */
	if (strcmp(fname_fit, "NONE") == 0) {
		arop_valid = 0;
		ncp = 0;
		rmse = 0;
		xshift = 0;
		yshift = 0;
	}
	else {
		double warpulx, warpuly;	/* For S2, they are the same as those for reference image*/
		double newwarpulx, newwarpuly;
		ret = read_aropstdout(fname_fit, 
					&warpulx, 
					&warpuly,
					&newwarpulx,
				     	&newwarpuly,
					&ncp,
					&rmse,
					coeff);

		if (ret != 0 || ncp < MINNCP) {
			//sprintf(message, "ncp is too small (%d), or error in read_aropstdout\n", ncp);
			//Error(message);
			arop_valid = 0;
			ncp = 0;
			rmse = 0;
			xshift = 0;
			yshift = 0;
		}
		else {
			arop_valid = 1;
			xshift = warpulx - newwarpulx;	/* only used for HDF attributes, not for calculation */
			yshift = warpuly - newwarpuly;
		}
	}

	if (!arop_valid) 
		dup_s2(&s2rin, &s2rout);
	else {
		int i, ib;
		int psi; 	/* Pixel size index: 0, 1, 2 for 10m, 20m, 60m respectively*/
		int irow, icol;
		int irowin, icolin;
		double subrowin, subcolin;	/* subpix location */
		int frow, fcol;			/* floor of the subpix location */
		double val[4][4];	 	/* to hold pixel values of any type */
		double xdis[4], ydis[4];	/* distance in the horizontal and vertical direction */
		double xw[4], yw[4];		/* weight */
		double conv;
		int bcol, ecol;
		int brow, erow;
		long kin, kout;
		char anyfill, anyset;
		unsigned char mask;
		unsigned char qaval;
		int aerosol[4];
		double pixsz[] = {10, 20, 60};	
		int bit;
		for (ib = 0; ib < S2NBAND; ib++) {
			psi = get_pixsz_index(ib); 
			for (irow = 0; irow < s2rout.nrow[psi]; irow++) {
				for (icol = 0; icol < s2rout.ncol[psi]; icol++) {
					kout = irow * s2rout.ncol[psi] + icol;
					
					/* AROP fitting is based on the 10m resolution. It can be applied to other 
					 * resolutions with the adjustment using the psi term.
					 */
					/* Nov 13, 2017: The second form seems to be easier to understand. 
					subcolin = coeff[0][0] / (pixsz[psi]/pixsz[0]) + coeff[0][1] * (icol+0.5) + coeff[0][2] * (irow+0.5);
					subrowin = coeff[1][0] / (pixsz[psi]/pixsz[0]) + coeff[1][1] * (icol+0.5) + coeff[1][2] * (irow+0.5);
					*/
					subcolin = coeff[0][0] * pixsz[0] / pixsz[psi] + coeff[0][1] * (icol+0.5) + coeff[0][2] * (irow+0.5);
					subrowin = coeff[1][0] * pixsz[0] / pixsz[psi] + coeff[1][1] * (icol+0.5) + coeff[1][2] * (irow+0.5);
					
					bcol = floor(subcolin+0.5)-2;
					brow = floor(subrowin+0.5)-2;
					ecol = bcol+3;
					erow = brow+3;
	
					/* Any of the input pixel is out of bound, no output */
					if (bcol < 0 || ecol > s2rin.ncol[psi]-1 || brow < 0 || erow > s2rin.nrow[psi]-1)
						continue;
	
	
					anyfill = 0;
					for (irowin = brow; irowin <= erow; irowin++) {
						for (icolin = bcol; icolin <= ecol; icolin++) {
							kin = irowin * s2rin.ncol[psi] + icolin;
							if (s2rin.ref[ib][kin] == HLS_S2_FILLVAL) 
								anyfill = 1;
							else
								val[irowin-brow][icolin-bcol] = s2rin.ref[ib][kin];
						}
					}
					
					if (anyfill)
						/* Actually already initialized to fill value. But for clarity, write explicitly. */
						s2rout.ref[ib][kout] = S2_ref_fillval;
					else {
						for (icolin = bcol; icolin <= ecol; icolin++) 
							xdis[icolin-bcol] = fabs(subcolin - (icolin+0.5));
						cc_weight(xdis, xw);
					
						for (irowin = brow; irowin <= erow; irowin++) 
							ydis[irowin-brow] = fabs(subrowin - (irowin+0.5));
						cc_weight(ydis, yw);

						conv = cubic_conv(val, xw, yw);
						s2rout.ref[ib][kout] = asInt16(conv);
					}
	
	
					/* Two masks. Only need this once; do it when the first 10m band is resampled,
					 * since the masks are at 10m.
					 */
					if (ib == 1) { 	/* 10m blue.  psi is 0 */
						/* If any of the four inner pixels in the 4x4 kernel is flagged in
						 * in a QA bit, set the bit for the output pixel.
						 */	 
						/* Oct 4, 2017: Now QA contains 10m LaSRC CLOUD information */
						/* Jul 9, 2019: ACmask is the renamed LaSRC CLOUD */


						/***** ACmask *****/
						mask = 0;

						/* Bits 0-5 are individual bits */
						for (bit = 0; bit <= 5; bit++) { 
							anyset = 0;
							for (irowin = brow+1; irowin <= erow-1; irowin++) {
								for (icolin = bcol+1; icolin <= ecol-1; icolin++) {
									kin = irowin * s2rin.ncol[psi] + icolin;
									if ((s2rin.acmask[kin] >> bit) & 01) {
										anyset = 1;
										break;
									}
								}
							}
		
							if (anyset)  
								mask = (mask | (01 << bit)); 
						}

						/* Bit 6-7 are a group on aerosol level: 4 levels. */
						aerosol[0] = aerosol[1] = aerosol[2] = aerosol[3] = 0;
						for (irowin = brow+1; irowin <= erow-1; irowin++) {
							for (icolin = bcol+1; icolin <= ecol-1; icolin++) {
								kin = irowin * s2rin.ncol[psi] + icolin;
								qaval = ((s2rin.acmask[kin] >> 6) & 03);
								aerosol[qaval]++;
							}
						}

						/* If a higher aerolsol level is present, use it for output.
						 * Four levels of aerosol levels: 0, 1, 2, 3 
						 */
						qaval = 0;
						for (i = 0; i <= 3; i++) {
							if (aerosol[i] > 0)
								qaval = i;
						}
						mask = (mask | (qaval << 6));

						/* Finally, assgin to output */
						s2rout.acmask[kout] = mask;

						/***** Fmask *****/
						mask = 0;

						/* Bits 0-5 are individual bits */
						for (bit = 0; bit <= 5; bit++) { 
							anyset = 0;
							for (irowin = brow+1; irowin <= erow-1; irowin++) {
								for (icolin = bcol+1; icolin <= ecol-1; icolin++) {
									kin = irowin * s2rin.ncol[psi] + icolin;
									if ((s2rin.fmask[kin] >> bit) & 01) {
										anyset = 1;
										break;
									}
								}
							}
		
							if (anyset)  
								mask = (mask | (01 << bit)); 
						}

						/* Bits 6-7 not set in Fmask */

						s2rout.fmask[kout] = mask;
					} 
				}
			}
		}
	}


	/* Copy input metadata, and add new about AROP*/
	copy_metadata(&s2rin, &s2rout);
	set_arop_metadata(&s2rout, fname_ref, ncp, rmse, xshift, yshift);

	/* HLS processing time */
	getcurrenttime(creationtime);
	SDsetattr(s2rout.sd_id, HLSTIME, DFNT_CHAR8, strlen(creationtime), (VOIDP)creationtime);

	/* Close. Set a few parameters for ENVI header */
	strcpy(s2rout.zonehem, s2rin.zonehem);
	s2rout.ulx = s2rin.ulx;
	s2rout.uly = s2rin.uly;
	ret = close_s2r(&s2rout);
	if (ret != 0) {
		Error("Error in close_sr");
		exit(1);
	}

	/* Make it hdfeos */
 	sds_info_t all_sds[S2NBAND+2];
	set_S10_sds_info(all_sds, S2NBAND+2, &s2rout);
	ret = S10_PutSpaceDefHDF(&s2rout, all_sds, S2NBAND+2);
	if (ret != 0) {
		Error("Error in HLS_PutSpaceDefHDF");
		exit(1);
	}

	return(0);
}


/* Copy the input metadata to output.  */
int copy_metadata(s2r_t *s2rin, s2r_t *s2rout)
{
	if (get_all_metadata(s2rin) != 0)
		return(1);
	
	SDsetattr(s2rout->sd_id, PRODUCT_URI, DFNT_CHAR8, strlen(s2rin->uri), (VOIDP)s2rin->uri);
	SDsetattr(s2rout->sd_id, L1C_QUALITY, DFNT_CHAR8, strlen(s2rin->quality), (VOIDP)s2rin->quality);
	SDsetattr(s2rout->sd_id, SPACECRAFT, DFNT_CHAR8, strlen(s2rin->spacecraft), (VOIDP)s2rin->spacecraft);
	SDsetattr(s2rout->sd_id, TILE_ID, DFNT_CHAR8, strlen(s2rin->tile_id), (VOIDP)s2rin->tile_id);
	SDsetattr(s2rout->sd_id, DATASTRIP_ID, DFNT_CHAR8, strlen(s2rin->datastrip_id), (VOIDP)s2rin->datastrip_id);
	SDsetattr(s2rout->sd_id, PROCESSING_BASELINE, DFNT_CHAR8, strlen(s2rin->baseline), (VOIDP)s2rin->baseline);
	SDsetattr(s2rout->sd_id, SENSING_TIME, DFNT_CHAR8, strlen(s2rin->sensing_time), (VOIDP)s2rin->sensing_time);
	SDsetattr(s2rout->sd_id, L1PROCTIME, DFNT_CHAR8, strlen(s2rin->l1proctime), (VOIDP)s2rin->l1proctime);
	SDsetattr(s2rout->sd_id, HORIZONTAL_CS_NAME, DFNT_CHAR8, strlen(s2rin->cs_name), (VOIDP)s2rin->cs_name);
	SDsetattr(s2rout->sd_id, HORIZONTAL_CS_CODE, DFNT_CHAR8, strlen(s2rin->cs_code), (VOIDP)s2rin->cs_code);
	SDsetattr(s2rout->sd_id, NROWS, DFNT_CHAR8, strlen(s2rin->nr), (VOIDP)s2rin->nr);
	SDsetattr(s2rout->sd_id, NCOLS, DFNT_CHAR8, strlen(s2rin->nc), (VOIDP)s2rin->nc);
	SDsetattr(s2rout->sd_id, SPATIAL_RESOLUTION, DFNT_CHAR8, strlen(s2rin->spatial_resolution), (VOIDP)s2rin->spatial_resolution);
	SDsetattr(s2rout->sd_id, ULX, DFNT_FLOAT64, 1, (VOIDP)&(s2rin->ululx));
	SDsetattr(s2rout->sd_id, ULY, DFNT_FLOAT64, 1, (VOIDP)&(s2rin->ululy));
	SDsetattr(s2rout->sd_id, MSZ, DFNT_FLOAT64, 1, (VOIDP)&(s2rin->msz));
	SDsetattr(s2rout->sd_id, MSA, DFNT_FLOAT64, 1, (VOIDP)&(s2rin->msa));
	SDsetattr(s2rout->sd_id, MVZ, DFNT_FLOAT64, 1, (VOIDP)&(s2rin->mvz));
	SDsetattr(s2rout->sd_id, MVA, DFNT_FLOAT64, 1, (VOIDP)&(s2rin->mva));

	SDsetattr(s2rout->sd_id, SPCOVER, DFNT_INT16, 1, (VOIDP)&(s2rin->spcover));
	SDsetattr(s2rout->sd_id, CLCOVER, DFNT_INT16, 1, (VOIDP)&(s2rin->clcover));
	SDsetattr(s2rout->sd_id, ACCODE,  DFNT_CHAR8, strlen(s2rin->accode), (VOIDP)s2rin->accode);

	return(0);
}

int set_arop_metadata(	s2r_t *s2rout,
			char *refimgname,
			int ncp,
			double rmse,
			double xshift,
			double yshift)
{
	/* Be careful:
	 * 
	 * Write an unterminated character array can ruin the whole file. Make sure every 
	 * character attribute contains a valid character string.
	 */
	SDsetattr(s2rout->sd_id, S_AROP_REFIMG, DFNT_CHAR8,   strlen(refimgname), (VOIDP)refimgname);
	SDsetattr(s2rout->sd_id, S_AROP_NCP,    DFNT_UINT32,  1, (VOIDP)&ncp);
	SDsetattr(s2rout->sd_id, S_AROP_RMSE,   DFNT_FLOAT64, 1, (VOIDP)&rmse);
	SDsetattr(s2rout->sd_id, S_AROP_XSHIFT, DFNT_FLOAT64, 1, (VOIDP)&xshift);
	SDsetattr(s2rout->sd_id, S_AROP_YSHIFT, DFNT_FLOAT64, 1, (VOIDP)&yshift);
	
	return(0);
}
