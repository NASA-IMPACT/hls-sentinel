/* Create 30m S2. 
 * The 10m bands are boxcar-averaged to 30m, the 60m bands are oversampled
 * to 30m (by splitting). The 20m bands are sampled to 30m area-weighted 
 * averge, rather than cubic convolution as in HLS V1.2. This way all three
 * resampling approaches have the same, simple geometric meaning. 
 * (It has been so since V1.3. Comment on Aug 29, 2017)
 */
#ifndef S2AT30M_H
#define S2AT30M_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mfhdf.h"
#include "error.h"
#include "hls_commondef.h"
//#include "cubic_conv.h"

#include "s2r.h" 

/* Aug 29, not used? */
//#define FNAME_MODISBRDF_BAND01 "fname_modisbrdf_band01"

/* No use to define the attribute names because files from upstream will
 * be opened for update, with attributes inherited.
 */
/* Almost the same as s2r_t, except that s2at30m_t only has one pixel-size */
typedef struct {
	char fname[LINELEN];
	intn access_mode;
	int nrow, ncol;		/* for the new 30m bands */
	char zonehem[10];
	double ulx;
	double uly;

	int32 sd_id;
	int32 sds_id_ref[S2NBAND];
	int32 sds_id_acmask;
	int32 sds_id_fmask;

	int16 *ref[S2NBAND];
	uint8 *acmask;
	uint8 *fmask;

	char tile_has_data;

	/* If the 30m reflectance is NBAR, the CMG BRDF filename will be written to the ouptut hdf.
	 * And a bit-packed flag is written indictate whether a band has been BRDF-corrected 
	 */
	/* Jul 15, 2019: comment out.  All pixels are corrected in David Roy's approach.
	char is_nbar[100]; 
	char fname_modisbrdf_band01[LINELEN];
	int32 sds_id_brdfflag;
	uint8 *brdfflag;
	*/

	/* spatial coverage and cloud coverage derived upstream */
	int16 spcover;
	int16 clcover;
} s2at30m_t;			/* S2 reflectance */

int open_s2at30m(s2at30m_t *s2at30m, intn access_mode); 
int close_s2at30m(s2at30m_t *s2at30m); 

/* Two function used to create 30m S2 from 10m, 20m, and 60m */
void dup_s2at30m(s2at30m_t *in, s2at30m_t *out);
int resample_s2to30m(s2r_t *s2r, s2at30m_t *s2at30m); 

#endif
