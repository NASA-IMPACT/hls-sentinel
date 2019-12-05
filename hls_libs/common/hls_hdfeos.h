/*  HDFEOS for S10, S30, and L30. */

#ifndef HLS_HDFEOS_H
#define HLS_HDFEOS_H

#include <math.h>
#include <mfhdf.h>
#include "util.h"
#include "s2r.h"
#include "s2at30m.h"
#include "lsat.h"

/* Purely for the sake of HDFEOS; HLS processing does not rely on this. 
 * This information is copied from an opened HLS file.
 */
typedef struct
{
	char name[200];
	char data_type_name[200];
	int32 data_type;
	char dimname[2][200];
	double ulx;
	double uly;
	int nrow;
	int ncol;
	double pixsz;
	int zonecode;
} sds_info_t;


typedef enum{false, true} bool;

#define MYHDF_MAX_NATTR_VAL 30000    /* Originally 3000; too small. Junchang Ju*/
#define NPROJ_PARAM_HDFEOS (13)
#define SPACE_STRUCT_METADATA ("StructMetadata.0")

/* Possible ranges for data types */

#define MYHDF_CHAR8H     (        255  )
#define MYHDF_CHAR8L     (          0  )
#define MYHDF_INT8H      (        127  )
#define MYHDF_INT8L      (       -128  )
#define MYHDF_UINT8H     (        255  )
#define MYHDF_UINT8L     (          0  )
#define MYHDF_INT16H     (      32767  )
#define MYHDF_INT16L     (     -32768  )
#define MYHDF_UINT16H    (      65535u )
#define MYHDF_UINT16L    (          0u )
#define MYHDF_INT32H     ( 2147483647l )
#define MYHDF_INT32L     (-2147483647l )
#define MYHDF_UINT32H    ( 4294967295ul)
#define MYHDF_UINT32L    (          0ul)
#define MYHDF_FLOAT32H   (3.4028234e+38f)
#define MYHDF_FLOAT32L   (1.1754943e-38f)
#define MYHDF_FLOAT64H   (1.797693134862316e+308)
#define MYHDF_FLOAT64L   (2.225073858507201e-308)


/* Structure to store information about the HDF attribute */
typedef struct {
  int32 id, type, nval; /* id, data type and number of values */
  char *name;           /* attribute name */
} Myhdf_attr_t;


/* Like a "privite" function */
bool AppendMeta(char *cbuf, int *ic, char *s);

bool PutAttrDouble(int32 sds_id, Myhdf_attr_t *attr, double *val);
bool PutAttrString(int32 sds_id, Myhdf_attr_t *attr, char *string);

//int PutSDSDimInfo(int32 sds_id, char *dimname, int irank);

/* S10 */
int set_S10_sds_info(sds_info_t *s2_sds, int nsds, s2r_t *s2r);
int S10_PutSpaceDefHDF(s2r_t *tile, sds_info_t sds[], int nsds);

/* S30 */
int set_S30_sds_info(sds_info_t *s2_sds, int nsds, s2at30m_t *s2r);
int S30_PutSpaceDefHDF(s2at30m_t *tile, sds_info_t sds[], int nsds);

/* L30 */
int set_L30_sds_info(sds_info_t *all_sds,  int nsds,  lsat_t *lsat);
int L30_PutSpaceDefHDF(lsat_t *tile, sds_info_t sds[], int nsds);
#endif
