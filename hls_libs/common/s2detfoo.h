/* Header file for S2 detector footprint image for all spectral bands.
 */

#ifndef S2DETFOO_H
#define S2DETFOO_H

#include <stdio.h>
#include <stdlib.h>
#include "mfhdf.h"
#include "util.h"
#include "fillval.h"
#include "hls_commondef.h"
#include "hdfutility.h"
#include "pnpoly.h"
#include "s2def.h"

#define NDETECTOR 12
#define DETIDFILL 0		/* Detectors are numbered 1-12 */
#define DETFOOPIXSZ 30 		/* 30m */


typedef struct {
	char fname[NAMELEN];
	intn access_mode;
	int nrow, ncol;		/* nrow, ncol for 60m bands */
	int zone;
	double ulx;
	double uly;

	int32 sd_id;
	int32 sds_id_detid;

	uint8 *detid;
} s2detfoo_t;			

/* open s2 detfoo for read or create */
int open_s2detfoo(s2detfoo_t *s2detfoo, intn access_mode);

/* Return 100 if the detfoo vector is not found in gml.  May 1, 2017. */
int rasterize_s2detfoo(s2detfoo_t *s2detfoo, char *fname_b01_gml);

/* Equally split the overlap area for each row of image.
 */
void split_overlap(s2detfoo_t *s2detfoo);

/* close */
int close_s2detfoo(s2detfoo_t *s2detfoo);

#endif
