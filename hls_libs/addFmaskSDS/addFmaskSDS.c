/******************************************************************************** 
 * The input is the result of LaSRC, and the output is the raw S10 (i.e., possible
 * twin granules are not consolidated yet). 
 *
 * 1. Add Fmask as a separate SDS named Fmask.
 *    Fmask for S2 was created at 20m. This code first dilates cloud and cloud shadow 
 *    by 7 pixels and then resamples and saves it at 10m resolution in the output for 
 *    easy evaluation with 10 visible bands. (This is S10.) 
 * 
 *    The reason for dilation by 7 20-m pixels is that after the Fmask is eventually
 *    resampled to 30m in S30 the dilation is equivalent to 5 30-m pixels. The same
 *    amount dilation will be applied for L30. (9/22/2020)
 * 2  Extract 2 bits of aerosol level from aerosol QA and add as bits 7-6 of Fmask. 
 *    (Apr 14, 2021)
 * 
 * 3. Atmospheric correction CLOUD SDS is no longer combined with Fmask, but is renamed ACmask. 
 *    Diable ACmask altogether by setting each pixel value to zero because the C LaSRC 
 *    does not generate CLOUD and the CLOUD passed in is fake and may not be zero for 
 *    each pixel.  (9/22/2020)  
 *
 * 4. Add spatial coverage attribute and add cloud cover attribute based on Fmask only.
 *    (ACmask has no effect)
 * 5. The output become S10, in HDF-EOS format.
 *
 * Note that earlier when the two files of AC output was combined the reflectance 
 * fillval was changed from -100 to -1000, and mask fillval from 24 to 255.  (Jun 27, 2019)
 *
 * Note: Jun 23, 2017
 * Can no longer create the output by opening the input surface reflectance 
 * image to update because the input now has a CLOUD SDS but the output is 
 * designed not to have a CLOUD SDS but a QA SDS. The CLOUD SDS is to be combined 
 * with Fmask to form a new QA SDS.  Also for metadata, since the inflexible function 
 * copy_metadata(s2r_t *s2rin, s2r_t *s2rout) was designed to copy 
 * all metadata include the AROP metadata which are not available at this point
 * yet, it cannot be used here. So add the metadata from two XML files again.
 *
 * Jun 25, 2019
 * Sep 22, 2020
 * Apr 14, 2021: extract 2 bits of aerosol level from aerosol QA and add as bits 7-6 of Fmask.
 ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "dilation.h"
#include "hls_projection.h"
#include "s2r.h"
#include "util.h"
#include "hls_hdfeos.h"

int copyref_addmask(s2r_t *s2in, char *fname_fmask, char *fname_aeroQA, s2r_t *s2_out);

int main(int argc, char *argv[])
{
	/* Command-line parameters */
	char fname_s2rin[500];		/* AC output, whose CLOUD SDS is to be renamed ACmask */
	char fname_fmask[500];		/* fmask result in flat binary */
	char fname_aeroQA[500];		/* Aerosol QA byte from USGS LaSRC */
	char fname_safexml[500];
	char fname_granulexml[500];
	char accodename[100];
	char fname_s2rout[500];		/* S10, its QA sds is a combination of CLOUD and Fmask */

	s2r_t s2rin; 			/* With CLOUD SDS */
	s2r_t s2rout;			/* S10 output, with ACmask and Fmask SDS */

	char creationtime[100];
	int ret;

	if (argc != 8) {
		fprintf(stderr, "%s s2rin fmask aeroQA safexml granulexml accodename s2rout \n", argv[0]);
		exit(1);
	}

	strcpy(fname_s2rin,   argv[1]);
	strcpy(fname_fmask,   argv[2]);
	strcpy(fname_aeroQA,  argv[3]);
	strcpy(fname_safexml, argv[4]);
	strcpy(fname_granulexml, argv[5]);
	strcpy(accodename,       argv[6]);
	strcpy(fname_s2rout,     argv[7]);

	/* Open the input for read*/
	strcpy(s2rin.fname, fname_s2rin);
	strcpy(s2rin.mask_unavailable, MASK_UNAVAILABLE);	/* No ACmask and Fmask to read yet */
	strcpy(s2rin.ac_cloud_available, AC_CLOUD_AVAILABLE);	/* AC CLOUD SDS is available for read*/
	ret = open_s2r(&s2rin, DFACC_READ);
	if (ret != 0) {
		Error("Error in open_s2r");
		exit(1);
	}

	/* Create the output. Default (without any falgs) is: Do not create CLOUD SDS, 
	 * but create ACmask and Fmask.
	 */
	strcpy(s2rout.fname, fname_s2rout);
	s2rout.nrow[0] = s2rin.nrow[0];
	s2rout.ncol[0] = s2rin.ncol[0];
	s2rout.ulx = s2rin.ulx;
	s2rout.uly = s2rin.uly;
	strcpy(s2rout.zonehem, s2rin.zonehem);
	ret = open_s2r(&s2rout, DFACC_CREATE);
	if (ret != 0) {
		Error("Error in open_s2r");
		exit(1);
	}

	/* Copy all the reflectance SDS from input and add ACmask and Fmask */
	/* Dilate Fmask (we don't have ACmask in USGS LaSRC. Oct 28, 2020 */
	ret = copyref_addmask(&s2rin, fname_fmask, fname_aeroQA, &s2rout);
	if (ret != 0) {
		Error("Error in copyref_addmask");
		exit(1);
	}

	/* Cannot copy all the metadata from s2rin because the cloud coverage attribute
	 * is not available in the s2rin. Reset all attributes.
  	 */
	if (setinputmeta(&s2rout, fname_safexml, fname_granulexml, accodename) != 0) {
		Error("Error in setinputmeta");
		exit(1);
	}

	/* Processing time */
	getcurrenttime(creationtime);
	SDsetattr(s2rout.sd_id, HLSTIME, DFNT_CHAR8, strlen(creationtime), (VOIDP)creationtime);

	/* spatial and cloud. Cloud coverage relies on QA SDS */
	setcoverage(&s2rout);

	if (close_s2r(&s2rout) != 0) {
		Error("Error in closing output");
		exit(1);
	}

	/* Make it hdfeos */
 	sds_info_t all_sds[S2NBAND+2];
	set_S10_sds_info(all_sds, S2NBAND+2, &s2rout);
	ret = S10_PutSpaceDefHDF(s2rout.fname, all_sds, S2NBAND+2); 
	if (ret != 0) {
		Error("Error in HLS_PutSpaceDefHDF");
		exit(1);
	}

	return 0;
}

int copyref_addmask(s2r_t *s2in, char *fname_fmask, char *fname_aeroQA, s2r_t *s2out)
{
	int ib;
	int psi;
	int k, npix;
	unsigned char mask, val;
	char message[MSGLEN];

	/* Needed for map projection */
	strcpy(s2out->zonehem, s2in->zonehem);
	s2out->ulx = s2in->ulx;
	s2out->uly = s2in->uly;

	for (ib = 0; ib < S2NBAND; ib++) {
		psi = get_pixsz_index(ib);
		npix = s2in->nrow[psi] * s2in->ncol[psi];
		for (k = 0; k < npix; k++)
			s2out->ref[ib][k] = s2in->ref[ib][k];
	}


	/***************************************************/
// Jun 27, 2019. This is the bit description of the v1.4 QA SDS. Although we do not keep this SDS
// in v1.5, we use the same bit description in v1.5 for both ACmask and Fmask.
//
//	/* v1.4 QA SDS */
//	/* Note: For better view with hdfdump, the blanks within the string is blank space characters, not tab */
// 	sprintf(attr,   "Bits are listed from the MSB (bit 7) to the LSB (bit 0): \n"
//                      "7-6    aerosol:\n"
//                      "       00 - climatology\n"
//                      "       01 - low\n"
//                      "       10 - average\n"
//                      "       11 - high\n"
//		  	"5      water\n"
//			"4      snow/ice\n"
//			"3      cloud shadow\n"
//    			"2      adjacent to cloud; \n",
//			"1      cloud\n"
//			"0      cirrus\n");
//	SDsetattr(s2r->sds_id_qa, "QA description", DFNT_CHAR8, strlen(attr), (VOIDP)attr);
//
//	/* The original CLOUD SDS from LaSRC 
//	CLOUD:QA index = "\n",
//	    		"\tBits are listed from the MSB (bit 7) to the LSB (bit 0):\n",
//	    		"\t7      internal test; \n",
//	    		"\t6      unused; \n",
//	    		"\t4-5     aerosol;\n",
//	    		"\t       00 -- climatology\n",
//	    		"\t       01 -- low\n",
//	    		"\t       10 -- average\n",
//	    		"\t       11 -- high\n",
//	    		"\t3      cloud shadow; \n",
//	    		"\t2      adjacent to cloud; \n",
//	    		"\t1      cloud; \n",
//	    		"\t0      cirrus cloud; \n",
//	*/

	/* v1.5 ACmask. Essentially CLOUD but with some bits swapped in compliance with the v1.4 QA SDS  */
	npix = s2in->nrow[0] * s2in->ncol[0];
	for (k = 0; k < npix; k++) { 
		/* Bug fix on Aug 30, 2019.  LaSRC S2 uses a baffling mask fill value 24, not 255! */
		//if (s2in->accloud[k] == HLS_MASK_FILLVAL)
		if (s2in->accloud[k] == AC_S2_CLOUD_FILLVAL)
			continue;

		mask = 0;

		/* Bits 0-3 are the same for CLOUD and ACmask.
		 * They are: CIRRUS, cloud, adjacent to cloud, and cloud shadow.
		 */
		mask = s2in->accloud[k] & 017; 	/* 017 is binary 1111 */

		/* Reserve bit 4 for snow/ice, which is not set in AC CLOUD  */

		/* Reserve bit 5 for water, which is not set in AC CLOUD. Although bit 7 of AC CLOUD
		 * is "internal test" for water (Oct 3, 2017), the quality is very poor.  Jun 27, 2019 
		 */

		/* 2-bit aerosol level, shift from original CLOUD bits 4-5 to ACmask bits 6-7 */
		mask = mask | (((s2in->accloud[k] >> 4 ) & 03) << 6);

		/* Finally */
		s2out->acmask[k] = mask;
	}


	/***** Fmask *****/

	int nrow10m, ncol10m;
	int nrow20m, ncol20m;
	int irow, icol, k10m, k20m;
	unsigned char *fmask;
	FILE *ffmask;

	nrow10m = s2in->nrow[0];
	ncol10m = s2in->ncol[0];

	/*** Fmask cloud mask is originally created at 20m, but oversampled here to 10m for S10 products. */
	nrow20m = s2in->nrow[0]/2;	/* 1/2 dimension of 10m bands */
	ncol20m = s2in->ncol[0]/2;
	if ((fmask = (uint8*)calloc(nrow20m * ncol20m, sizeof(uint8))) == NULL) {
		Error("Cannot allocate memory\n");
		return(1);
	}
	if ((ffmask = fopen(fname_fmask, "r")) == NULL) {
		sprintf(message, "Cannot read Fmask %s\n", fname_fmask);
		Error(message);
		return(1);
	}
	if (fread(fmask, sizeof(uint8), nrow20m * ncol20m, ffmask) !=  nrow20m * ncol20m) {
		sprintf(message, "Fmask file size wrong: %s\n", fname_fmask);
		Error(message);
		return(1);
	}
	fclose(ffmask);

	/* Dilate 20m Fmask result by 7 pixels.  9/22/2020 */
	dilate(fmask, nrow20m, ncol20m, 7);

	/* Read USGS aerosol QA byte. Apr 14, 2021*/
	FILE *faeroQA;
	unsigned char *aeroQA;
	if ((aeroQA = (uint8*)calloc(nrow10m * ncol10m, sizeof(uint8))) == NULL) {
		Error("Cannot allocate memory\n");
		return(1);
	}
	if ((faeroQA = fopen(fname_aeroQA, "r")) == NULL) {
		sprintf(message, "Cannot read Fmask %s\n", fname_aeroQA);
		Error(message);
		return(1);
	}
	if (fread(aeroQA, sizeof(uint8), nrow10m * ncol10m, faeroQA) !=  nrow10m * ncol10m) {
		sprintf(message, "Fmask file size wrong: %s\n", fname_aeroQA);
		Error(message);
		return(1);
	}
	fclose(faeroQA);

	for (irow = 0; irow < nrow10m; irow++) {
		for (icol = 0; icol < ncol10m; icol++) {
			k10m = irow * ncol10m + icol;
			k20m = (irow/2) * ncol20m + (icol/2);

			if (fmask[k20m] == HLS_MASK_FILLVAL)	/* Fmask has used 255 as fill */
				continue;

			/* fmask
			clear land = 0
			water = 1
			cloud shadow = 2
			snow = 3
			cloud = 4
			thin_cirrus = 5		#  Seems cirrus is dropped in v4.0?   Mar 20, 2019 
			*/

			switch(fmask[k20m])
			{
				case 254:	/* Dilated cloud or cloud shadow */
					val = 1;
					mask = (val << 2);
					break;
				case 5: 	/* CIRRUS. But not set in Fmask4.0. A placeholder for now. Jun 27, 2019 */
					mask = 1;
					break;
				case 4: 	/* cloud*/ 
					val = 1;
					mask = (val << 1);
					break;
				case 3: 	/* snow/ice. */
					val = 1;
					mask = (val << 4);
					break;
				case 2: 	/* Cloud shadow */
					val = 1;
					mask = (val << 3);
					break;
				case 1: 	/* water*/
					val = 1;
					mask = (val << 5);
					break;
				case 0:
					mask = 0;
					break;	/* clear; do nothing */
				default: 
					sprintf(message, "Fmask value not expected: %d", fmask[k20m]);
					Error(message);
					exit(1);
			}

			/* Add the 2 bits of aerosol level from USGS aerosol QA */
			mask = mask | (((aeroQA[k10m] >> 6 ) & 03) << 6);
			s2out->fmask[k10m] = mask;
		}
	}

	return 0;
}
