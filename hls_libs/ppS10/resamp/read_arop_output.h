#ifndef READ_AROP_H
#define READ_AROP_H

#include "hls_commondef.h"
#include "util.h"

/* Aug 29, 2017: 
 * Comment out; use whatever number of control points, as long as AROP does not warn 
 * Sep 18, 2017: Use it
*/
#define MINNCP 10	/* At least this many control points are needed to use AROP results */


/* Read the AROP output to stdout, which contains everything.  No need to read the log file. 
 * 
 * Return 0 for success, and non-zero for failure.  Possible failure: no enough control points, what else?
 * 
 */
int read_aropstdout(char *fname_arop, 
		    double *warpulx,
		    double *warpuly,
		    double *newwarpulx,
		    double *newwarpuly,
		    int *ncp,
		    double *rmse,
		    double coeff[2][3]);


#endif
