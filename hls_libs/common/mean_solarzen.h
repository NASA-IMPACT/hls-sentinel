#ifndef MEAN_SOLARZEN_H
#define MEAN_SOLARZEN_H

#include "hls_projection.h"

/* Move the SDSU definitions from main() to here.  JU. Aug 2, 2019.
 *
 * Mean zenith at L8 and S2 overpass time.
 */
static double l8_inclination = 98.2;
static double l8_Local_Time_at_Descending_Node=10.18333333333;
static double s2_inclination = 98.62;
static double s2_Local_Time_at_Descending_Node=10.5;

/* Li refers to the Zhongbin Li */
double mean_solarzen(char *utm_numhem, double ux, double uy, int year, int doy);
double Li_solarzen(double inclination, double Local_Time_at_Descending_Node, char *utm_numhem, double ux, double uy, int iYear, int mdoy1);

#endif
