#ifndef CUBIC_CONV_H
#define CUBIC_CONV_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "util.h"

/* Compute cubic convolution weights from the horizontal or vertical distance.
 * Return non-zero on error
 */
int cc_weight(double dis[4], double w[4]);

/* Compute cubic convolution */
double cubic_conv(double val[][4], double xw[4], double yw[4]);

#endif
