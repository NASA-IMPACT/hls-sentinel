/* Derive the solar zenith/azimuth and view zenith/azimuth for every 30m pixel.
 * The view zenith/azimuth are interpolated from the 5km data for B06 and used
 * for all bands.
 * 
 * 1 December, 2021
 * This process needs the detector footprint image to cookie-cut the interpolated
 * angle image.  
 * For Sentinel-2 PB before 4.0, the footprint is derived from the B06 footprint GML.
 * And for PB 4.0 and after, read the B06 footprint image directly;  the image was
 * ENVI plain binary converted from ESA JP2.
 */

#include <stdio.h>
#include <stdlib.h>
#include "hls_commondef.h"
#include "s2mapinfo.h"
#include "s2detfoo.h"
#include "s2ang.h"
#include "util.h"
#include "hls_hdfeos.h"


/* Return -1 if the detfoo vector is not available in the DETFOO gml.  May 1, 2017 */
int main(int argc, char *argv[])
{
	char fname_xml[LINELEN];	/* angle is in this granule xml*/
	char fname_b06_detfoo[LINELEN];	/* Either gml or plain binary for B06, at 20m */ 
	char fname_detfoo[LINELEN]; 	/* detfoo is created at 30m and saved by this code */
	char fname_ang[LINELEN]; 	/* angle output */

	s2mapinfo_t mapinfo;
	s2detfoo_t s2detfoo;
	s2ang_t s2ang;

	char message[MSGLEN];
	int ret;
	char *pos;

	if (argc != 5) {
		fprintf(stderr, "%s in.granule.xml in.detfoo.20m out.detfoo.30m.hdf out.ang.hdf\n", argv[0]);
		exit(1);
	}

	strcpy(fname_xml, argv[1]);
	strcpy(fname_b06_detfoo, argv[2]);
	strcpy(fname_detfoo, argv[3]);
	strcpy(fname_ang, argv[4]);

	/* Read the map info */
	read_s2mapinfo(fname_xml, &mapinfo);

	/* Create an empty footprint file */
	strcpy(s2detfoo.fname, fname_detfoo);
	s2detfoo.nrow = mapinfo.nrow[2] * (60.0 / DETFOOPIXSZ);	/* index 2 is for 60m */
	s2detfoo.ncol = s2detfoo.nrow; 
	s2detfoo.ulx = mapinfo.ulx;
	s2detfoo.uly = mapinfo.uly;
	ret = open_s2detfoo(&s2detfoo, DFACC_CREATE);
	if (ret != 0) {
		exit(1);
	}
	
	/* For processing baseline preceding 4.0, rasterize the footprint vector given in GML,
	 * and then split the overlap region evenly if needed (for the initial few years 
	 * there was overlap in footprint).
	 * For PB4.0 and later, read the footprint image directly.
	 * 
	 * Note that the corners in the GML refer to the footprint extent, not the tile extent.
	 */ 

	if (strstr(fname_b06_detfoo, "B06.gml")) {
		ret = rasterize_s2detfoo(&s2detfoo, fname_b06_detfoo);
		if (ret != 0) {
			sprintf(message, "Error in deriving detfoo: %s", fname_b06_detfoo);
			Error(message);
			exit(ret);
		}
		split_overlap(&s2detfoo);	// Nov 28, 2019. No overlap any more.
	}
	else if (strstr(fname_b06_detfoo, "B06.bin")) {
		FILE *df;
		uint8  *detid;

		int irow, icol, row20m, col20m;
		int k; 
		if ((df = fopen(fname_b06_detfoo, "r")) == NULL) {
			sprintf(message, "Cannot open %s", fname_b06_detfoo);
			Error(message);
			exit(1);
		}

		/*  index 1 in mapinfo is for 20m bands including B06 */
		k = mapinfo.nrow[1] * mapinfo.ncol[1];
		if ((detid = (uint8*)calloc(k, sizeof(uint8))) == NULL) {
			Error("Cannot allocate memory");
			exit(1);
		}
		if (fread(detid, sizeof(uint8), k, df) != k) {
			sprintf(message, "File size is wrong, too short: %s", fname_b06_detfoo);
			Error(message);
			exit(1);
		}

		for (irow = 0; irow < s2detfoo.nrow; irow++) {
			row20m = floor((irow + 0.5) * DETFOOPIXSZ / 20.0); /* 20 is B06 pixel size  */
			for (icol = 0; icol < s2detfoo.ncol; icol++) {
				col20m = floor((icol + 0.5) * DETFOOPIXSZ / 20.0); /* 20 is B06 pixel size  */
				s2detfoo.detid[irow*s2detfoo.ncol+icol] = detid[row20m * mapinfo.ncol[1] + col20m]; 
			}
		}
		free(detid);
	}
	else {
		sprintf(message, "Input is neither gml nor bin for B06: %s", fname_b06_detfoo);
		Error(message);
		exit(1);
	}

	/* Create an empty angle file */
	strcpy(s2ang.fname, fname_ang);
	s2ang.nrow = mapinfo.nrow[2] * (60.0 / DETFOOPIXSZ);	/* index 2 is for 60m */
	s2ang.ncol = s2ang.nrow; 
	strcpy(s2ang.zonehem, mapinfo.zonehem);
	s2ang.ulx = mapinfo.ulx;
	s2ang.uly = mapinfo.uly;
	ret = open_s2ang(&s2ang, DFACC_CREATE);
	if (ret != 0) {
		exit(1);
	}

	/* Read the 5-km angle from xml and interpolate */
	ret = make_smooth_s2ang(&s2ang, &s2detfoo, fname_xml);
	if (ret != 0) {
		Error("error in make_smooth_s2ang");
		exit(ret);
	}

	/* close */
	ret = close_s2detfoo(&s2detfoo);
	if (ret != 0)
		exit(1);

	ret = close_s2ang(&s2ang);
	if (ret != 0)
		exit(1);


	/* Make it hdfeos */
        if (strstr(mapinfo.zonehem, "S")) 
             	s2ang.uly -= 1e7;		// To GCTP (and HDF-EOS?) convention.
	sds_info_t all_sds[NANG];
        set_S2ang_sds_info(all_sds, NANG, &s2ang);
        ret = angle_PutSpaceDefHDF(s2ang.fname, all_sds, NANG);
        if (ret != 0) {
                Error("Error in HLS_PutSpaceDefHDF");
                exit(1);
        }

	return 0;
}
