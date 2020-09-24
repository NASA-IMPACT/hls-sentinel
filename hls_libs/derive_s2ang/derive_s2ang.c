/* Derive the solar zenith/azimuth and view zenith/azimuth for every 30m pixel.
 * The view zenith/azimuth are interpolated from the 5km data for B06.
 * 
 * Note that the filename of footprint gml for B01 is passed in and from that
 * the filename for B06 is derived. The use of B06 for all bands  is a recent 
 * change and therefore on the commandline B01 fileanme is still used just to 
 * the keep interface the same, for the IMPACT team.
 * 9/23/2020
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
	char fname_b01_gml[LINELEN];	/* Use b01 detfoo gml to find gml of other bands*/
	char fname_detfoo[LINELEN]; 	/* detfoo is created and saved by this code */
	char fname_ang[LINELEN]; 	/* angle output */

	s2mapinfo_t mapinfo;
	s2detfoo_t s2detfoo;
	s2ang_t s2ang;

	char message[MSGLEN];
	int ret;
	char *pos;

	if (argc != 5) {
		fprintf(stderr, "%s in.granule.xml in.detfoo.b01.gml out.detfoo.hdf out.ang.hdf\n", argv[0]);
		exit(1);
	}

	strcpy(fname_xml, argv[1]);
	strcpy(fname_b01_gml, argv[2]);
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
	
	/* Rasterize the footprint vector, and split the overlap region evenly.
	 * Note that the corners in the GML refer to the footprint extent, not the tile extent.
	 * Use B01 footprint filename to find names of all other bands.
	 * Actually now a single band B06 is selected. But still keep the function interface
	 * to pass in B01 filename. 
	 */ 
	ret = rasterize_s2detfoo(&s2detfoo, fname_b01_gml);
	if (ret != 0) {
		sprintf(message, "Error in deriving detfoo with b01 name: %s", fname_b01_gml);
		Error(message);
		exit(ret);
	}
	split_overlap(&s2detfoo);	// Nov 28, 2019. No overlap any more.

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
        ret = S2ang_PutSpaceDefHDF(&s2ang, all_sds, NANG);
        if (ret != 0) {
                Error("Error in HLS_PutSpaceDefHDF");
                exit(1);
        }

	return 0;
}
