#ifndef S2MAPINFO_H
#define S2MAPINFO_H

/* Read a granule's XML file to get the dimension of the image.
   This is needed by the S2 angle derivation code. We can go without
   this if the image dimension is hard coded.
 */

typedef struct {
	int nrow[3];	/* nrow for 10m, 20m, and 60m bands */
	int ncol[3]; 	/* ncol for 10m, 20m, and 60m bands */
	char zonestr[10];
	double ulx;
	double uly;
} s2mapinfo_t;


/* Read mapinfo from xml file. Return 0 for success and 
 * non-zero for error.
 */
int read_s2mapinfo(char *fname_xml, s2mapinfo_t *mapinfo);

#endif
