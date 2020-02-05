#ifndef PNPOLY_H
#define PNPOLY_H

/* Test if a point is in a polygon.
 * Source: http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
 * But change float to double.
 */

/* Return 0 if not in, nonzero if in */
int pnpoly(int nvert, double *vertx, double *verty, double testx, double testy);

int pnpoly2(int nvert, double *vertx, double *verty, double testx, double testy);

#endif
