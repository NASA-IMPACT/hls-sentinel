/* Header file for S2 sun-view angles.
 * Modified on Sep 7, 2020: same view zenith and azimuth for all bands.
 *   Use the angle of the middle red edge band.
 */
#ifndef S2ANG_H
#define S2ANG_H

#include <stdio.h>
#include <stdlib.h>
#include "mfhdf.h"
#include "util.h"
#include "fillval.h"
#include "hls_commondef.h"
#include "hdfutility.h"
#include "s2def.h"
#include "s2detfoo.h"


#define NANG  4 	/* Four angle SDS */

static char *ANG_SDS_NAME[] = {
		"solar_zenith",
		"solar_azimuth",
		"view_zenith",
		"view_azimuth"};

#define ANGPIXSZ DETFOOPIXSZ    /* same as detfoo pixel size */
#define N5KM 23			/* 23 x 23 points with angle info, every 5km*/
typedef struct {
	char fname[NAMELEN];		
	intn access_mode;
	int nrow, ncol;		
	char zonehem[10];	/* e.g.  UTM zone and N/S hemisphere, 10S */
	double ulx;
	double uly;

	int32 sd_id;
	int32 sds_id[NANG];

	/* Jan 22, 2016. Change data type from int16 to uint16 because potentially a 
	 * view azimuth can be close to 360 with a scaled value  36000 which is greater than int16 max.
	 * 
	 * Sep 23, 2020: vz and va for B06 and used for all other bands.
	 */
	uint16 *ang[NANG];

} s2ang_t;			


/* open s2 angles for read or create */
int open_s2ang(s2ang_t *s2ang, int access_mode); 

/* Return 101 if the xml format is wrong. May 1, 2017 */
int make_smooth_s2ang(s2ang_t *s2ang, s2detfoo_t *s2detfoo, char *fname_xml);
int interp_s2ang_bilinear(uint16 *ang, int nrow, int ncol);

/* close */
int close_s2ang(s2ang_t *s2ang);

#endif
