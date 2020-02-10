/* The ratio used in BRDF adjustment for each band of S2 or L8 */

#ifndef CFACTOR_H
#define CFACTOR_H

#include <stdio.h>
#include <stdlib.h>
#include "mfhdf.h"
#include "error.h"
#include "hls_commondef.h"
#include "hdfutility.h"
#include "hls_projection.h"

#define LANDSAT8  1
#define SENTINEL2 2

#define NBAND_CFACTOR_L8 7	/* Drop cirrus band */
#define NBAND_CFACTOR_S2 11	/* Drop water vapor and cirrus bands */
#define NBAND_CFACTOR	NBAND_CFACTOR_S2	/* Use the max of the two to define the arrays */

typedef struct {
	char fname[LINELEN];
	intn access_mode;
	int nband;	/* is NBAND_CFACTOR */
	int nrow;
	int ncol;

	char sds_name[NBAND_CFACTOR][50];
	int32 sd_id;
	int32 sds_id_ratio[NBAND_CFACTOR];
	float32 *ratio[NBAND_CFACTOR];

} cfactor_t;


int open_cfactor(int sensor_type, cfactor_t *cfact, intn access_mode);

int close_cfactor(cfactor_t *cfact);

#endif
