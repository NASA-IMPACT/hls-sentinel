/* Header file for S2 sun-view angles.
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


#define SZ "solar_zenith"
#define SA "solar_azimuth"
#define VZ "view_zenith"
#define VA "view_azimuth"

#define ANGPIXSZ DETFOOPIXSZ    /* same as detfoo pixel size */
#define N5KM 23			/* 23 x 23 points with angle info, every 5km*/
#define ANGLEAVAIL "ANGLEAVAIL"
typedef struct {
	char fname[NAMELEN];		
	intn access_mode;
	int nrow, ncol;		
	int zone;
	double ulx;
	double uly;

	int32 sd_id;
	int32 sds_id_sz;
	int32 sds_id_sa;
	int32 sds_id_vz[S2NBAND];
	int32 sds_id_va[S2NBAND];

	/* Jan 22, 2016. Change data type from int16 to uint16 because potentially a 
	 * view azimuth can be close to 360 with a scaled value  36000 which is greater than int16 max.
	 */
	uint16 *sz;
	uint16 *sa;
	uint16 *vz[S2NBAND];	/* vz and va vary with band for the same location. */
	uint16 *va[S2NBAND];
	uint8 angleavail[S2NBAND];
	/* Bug fix on Sep 9, 2016: The 13-element indicator array 'angleavail' indicates 
	 * whether view angles are available for each MSI band. This information will be 
	 * saved as an HDF attribute named ANGLEAVAIL.
 	 */
} s2ang_t;			


/* open s2 angles for read or create */
int open_s2ang(s2ang_t *s2ang, int access_mode); 

/* Return 101 if the xml format is wrong. May 1, 2017 */
int make_smooth_s2ang(s2ang_t *s2ang, s2detfoo_t *s2detfoo, char *fname_xml);
int interp_s2ang_bilinear(uint16 *ang, int nrow, int ncol);

/* If the view zenith or azimuth SDS for a band is missing, find a substitute band and 
 * save its bandId.  If it is not missing, use its own bandId.
 * Sep 9, 2016. If the ESA input is complete, no subtitution will be needed.
 * Used in NBAR calculation.
 *
 * Input:: angleavail[] indicates whether the view angle for each band is available (from XML)
 * Output:: subId is the bandId of the substitutes. It is possible that all bands are missing
 *          and no substitute can be found.
 * 
 * When looking for the substitute, try the bands in the same focal plane first; bands in
 * in the other focal plane are the last resort.
 */
void find_substitute(uint8 *angleavail, uint8 *subId);

/* close */
int close_s2ang(s2ang_t *s2ang);

#endif
