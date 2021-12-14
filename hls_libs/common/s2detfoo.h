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

/********************************************************************************
 * Generate the B06 detector footprint image by rasterizing the footprint vector 
 * polygon (before PB4.0).
 *
 * If there is detector footprint overlap (much earlier than PB 4.0),  write at the 
 * relevant pixels the number id_left * NDETECTOR + id_right 
 * (id_left and id_right are the detector id on the left and right respectively.)
 * Later on, another function will evenly split the overlap between two detectors.
 *
 * Return 100 if the detfoo vector is not found.  May 1, 2017.
 *
 * Use B06 for all other bands. Sep 7, 2020.
 */
int rasterize_s2detfoo(s2detfoo_t *s2detfoo, char *fname_b01_gml);

/* Equally split the overlap area for each row of image.
 */
void split_overlap(s2detfoo_t *s2detfoo);

/* close */
int close_s2detfoo(s2detfoo_t *s2detfoo);

#endif
