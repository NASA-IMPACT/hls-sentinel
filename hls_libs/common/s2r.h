/* Header file for an S10 input/output S2 image.  */
#ifndef S2R_H
#define S2R_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mfhdf.h"
#include "hls_commondef.h"
#include "hdfutility.h"
#include "s2def.h"
#include "fillval.h"
#include "util.h"


/* Spectral SDS names used by the AC code; will switch to HLS name 
 * defined in s2def.h
 * Why not combine s2r.h and s2def.h?  Aug 22, 2017.
 */
static char *VermoteS2sdsname[] = { 
		      "band01", 
		      "blue",  
		      "green",
		      "red", 
		      "band05",
		      "band06",
		      "band07",
		      "band08",
		      "band8a",
		      "band09",
		      "band10",
		      "band11",
		      "band12" };	
/* Jul 27, 2017. The new code produces a new SDS named CLOUD */
#define AC_CLOUD_NAME  "CLOUD"	

/* Output from Vermote code does not have the two HLS mask SDS, but a CLOUD SDS.
 * The default is to ignore AC CLOUD but use ACmask and Fmask.
 */
#define AC_CLOUD_AVAILABLE  "ac_cloud_available"	  
#define MASK_UNAVAILABLE "mask_unavailable"	 

/* Metadata items */
#define PRODUCT_URI "PRODUCT_URI"
#define L1C_QUALITY  "L1C_IMAGE_QUALITY"   /* A concatenation of severl quality flags */
#define SPACECRAFT   "SPACECRAFT_NAME"
#define PROCESSING_BASELINE "PROCESSING_BASELINE"
#define TILE_ID   "TILE_ID"
#define DATASTRIP_ID   "DATASTRIP_ID"
#define SENSING_TIME   "SENSING_TIME"
#define ARCHIVING_TIME "ARCHIVING_TIME"
#define L1PROCTIME     "L1_PROCESSING_TIME"	/* Use ARCHIVING_TIME for L1PROCTIME */
#define HORIZONTAL_CS_NAME "HORIZONTAL_CS_NAME"
#define HORIZONTAL_CS_CODE "HORIZONTAL_CS_CODE"
#define NROWS  "NROWS"
#define NCOLS  "NCOLS"
#define SPATIAL_RESOLUTION "SPATIAL_RESOLUTION"
#define ULX "ULX"
#define ULY "ULY"
#define MSZ  "MEAN_SUN_ZENITH_ANGLE(B01)"
#define MSA  "MEAN_SUN_AZIMUTH_ANGLE(B01)"
#define MVZ  "MEAN_VIEW_ZENITH_ANGLE(B01)"
#define MVA  "MEAN_VIEW_AZIMUTH_ANGLE(B01)"
#define HLSTIME "HLS_PROCESSING_TIME"
#define SPCOVER  "spatial_coverage"
#define CLCOVER  "cloud_coverage"
#define ACCODE   "ACCODE"

#define ACMASK_NAME "ACmask"
#define FMASK_NAME "Fmask"

#define S_AROP_REFIMG "arop_s2_refimg"
#define S_AROP_NCP "arop_ncp"
#define S_AROP_RMSE "arop_rmse(meters)"
#define S_AROP_XSHIFT "arop_ave_xshift(meters)"
#define S_AROP_YSHIFT "arop_ave_yshift(meters)"

typedef struct {
	char fname[NAMELEN];		/* Currently b01 plain binary filename */
	intn access_mode;
	int nrow[3];		/* For 10m, 20m, 60m */
	int ncol[3];
	char zonehem[10];	/* e.g.  UTM zone and N/S hemisphere, 10S */
	float64 ulx;
	float64 uly;

	int32 sd_id;
	int32 sds_id_ref[S2NBAND];
	int16 *ref[S2NBAND];

	/* Read AC CLOUD SDS */
	char ac_cloud_available[100]; 
	int32 sds_id_accloud;	
	uint8 *accloud; 

	/* ACmask and Fmask */ 
	char mask_unavailable[100]; /* ACmask and Fmask not available yet, i.e. this is output from AC code */
	int32 sds_id_acmask;	/* An exact copy of the CLOUD SDS from AC, but renamed */
	uint8 *acmask;	
	int32 sds_id_fmask;	/* Fmask */
	uint8 *fmask;	


	/* Used as set/get the attributes of the image, from SAFE xml and granule xml */
	char uri[300];		/* Product URI, from SAFE XML */ 
	char quality[300];	/* Quality flag, from SAFE XML */ 
	char spacecraft[20];	/* Same for the same datastrip */
	char granuleid[300];   /* e.g. 11SKA */
	char tile_id[300];	/* Jan 18, 2017: It seems that the definitions of tile and granule are swappped incorrectly. */
	char datastrip_id[300];
	char baseline[50]; 	/* processing baseline */
	char sensing_time[100];
	char l1proctime[100];
	char cs_name[100];
	char cs_code[100];
	char nr[60]; /* saved as strings. "10980/5660/1830" */
	char nc[60]; /* saved as strings. "10980/5660/1830" */
	char spatial_resolution[40];  /* as strings */
	float64 ululx;	/* upper-left corner of the upper-left pixel. Same as ulx */
	float64 ululy;
	float64 msz; /* four mean angles for B01 */
	float64 msa;
	float64 mvz;
	float64 mva;
	int16 spcover;		/* Spatial coverage in percentage */
	int16 clcover; 		/* Cloud coverage in percentage, if present in either ACmask or Fmask */
	/* Mar 21, 2016: spcover and clcover were originally defined as uint8 but then
	   hdfncump show them as characters. hdfncdump is not portable.
	*/
	/* Dec 26, 2016: AROP related metadata. If refimg contains "NONE", they are not available. */
	char refimg[300];
	int ncp;
	double rmse;
	double xshift;
	double yshift;
	char accode[100];	/* Atmospheric correction code name. 11/21/17 */

	char tile_has_data;	/* Indicate whether a created tile has data */

} s2r_t;			/* S2 reflectance */


/* Spectral values of a S2 pixel. Might use in cubic convolution. Not needed now. */
typedef struct {
	int16 ref[S2NBAND];
} s2r_pix_t;		

/* open s2 reflectance for read or create*/
int open_s2r(s2r_t *s2r, intn access_mode);

/* close */
int close_s2r(s2r_t *s2r);

/* Duplicate in to out */
void dup_s2(s2r_t *in, s2r_t *out);

/*******************************************************************************/
/*  A few functions that is "private" in C++ terminology; it is used by the    */
/*  above interface funtions.						       */
/*******************************************************************************/
/* S2 data are available at three different pixel sizes, 10m, 20m, and 60m.
   For a given band index, the corresponding index of its pixel size class in various 
   pixel-size-related arrays need to be found: 10m at location 0, 20m at 1, and 60m at 2. 
   The input band index ranges from 0 to 12, corresponding to the almost the 
   band numbers in the wavelength order; in particular, band 8a is given an index 8. 
*/
int get_pixsz_index(int bandidx); 

/* Add input metadata */
/* Nov 15, 2016: No longer add SAFE name as attribute 
 * Jan 19, 2017: Add quality flag from the SAFE XML.
 * Nov 21, 2017: Add accode: atmospheric correction code name 
*/
int setinputmeta(s2r_t *s2r,  char *safexml, char *granulexml, char *accode);
int get_all_metadata(s2r_t *s2r);

/* Aug 29, 2017: Comment out. Not used? */
//int copy_metadata(s2r_t *s2r, s2r_t *s2rout);

/* Spaital and cloud coverage of a tile in percentage. SDS qa should be set before doing this */
void setcoverage(s2r_t *s2r);

#endif
