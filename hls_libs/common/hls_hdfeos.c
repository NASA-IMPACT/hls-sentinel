/*********************************************************************************
    Convert the data to HDF-EOS format using the basic but flexible HDF API, rather 
    than the HDF-EOS interface, which has very limited capability of writing attributes 
    for the individual SDS and the whole file.  
    
    Adapted from LEDAPS space.c

	2010 for WELD.
	Sep 26, 2016.

	Oct 17, 2017: Bug fix by Zhangshi Yin
*/

#include "hls_hdfeos.h"
#include "util.h"
int set_S10_sds_info(sds_info_t *all_sds,  int nsds,  s2r_t *s2r)
{
	int iband;
	int psi;	/* Pixel size index; 0 for 10m, 1 for 20m, 2 for 60m */
	char message[MSGLEN];

	char *dimnames[][2] =  {{"YDim_Grid_10m", "XDim_Grid_10m"},
				{"YDim_Grid_20m", "XDim_Grid_20m"},
				{"YDim_Grid_60m", "XDim_Grid_60m"}};
	double pixsz[] = {10, 20, 60};

	if (nsds == S2NBAND || nsds == S2NBAND+2) {
		/* The 13 bands of S2 */
		for (iband = 0; iband < S2NBAND; iband++) {
			strcpy(all_sds[iband].name,  S2_SDS_NAME[iband]);
			strcpy(all_sds[iband].data_type_name, "DFNT_INT16");
			all_sds[iband].data_type = DFNT_INT16;
			psi = get_pixsz_index(iband);
			strcpy(all_sds[iband].dimname[0], dimnames[psi][0]);
			strcpy(all_sds[iband].dimname[1], dimnames[psi][1]);

			all_sds[iband].ulx = s2r->ulx;
			all_sds[iband].uly = s2r->uly;
			all_sds[iband].nrow = s2r->nrow[psi];
			all_sds[iband].ncol = s2r->ncol[psi];
			all_sds[iband].pixsz = pixsz[psi];
			all_sds[iband].zonecode =  atoi(s2r->zonehem);
		}

		/* Masks */
		if (nsds == S2NBAND+2) {
			/* ACmask */
			strcpy(all_sds[nsds-2].name,  ACMASK_NAME);
			strcpy(all_sds[nsds-2].data_type_name, "DFNT_UINT8");
			//all_sds[iband].data_type = DFNT_UINT8;	/* v1.4; not wrong but not clear. Jun 27, 2019 */
			all_sds[nsds-2].data_type = DFNT_UINT8;
			strcpy(all_sds[nsds-2].dimname[0], dimnames[0][0]);
			strcpy(all_sds[nsds-2].dimname[1], dimnames[0][1]);

			/* Fmask */
			strcpy(all_sds[nsds-1].name,  FMASK_NAME);
			strcpy(all_sds[nsds-1].data_type_name, "DFNT_UINT8");
			all_sds[nsds-1].data_type = DFNT_UINT8;
			strcpy(all_sds[nsds-1].dimname[0], dimnames[0][0]);
			strcpy(all_sds[nsds-1].dimname[1], dimnames[0][1]);

			/* Map projection parameters not set here because they are same as for other SDS. 
			 * Oct 18, 2018 */
		}
	}
	else {
		sprintf(message, "Number of Sentinel-2 SDS is neither %d nor %d",  S2NBAND, S2NBAND+2);
		Error(message);
		return(1);
	}


	return(0);
}

int S10_PutSpaceDefHDF(char *hdfname, sds_info_t sds[], int nsds)
{
	int32 sd_id;
	char struct_meta[MYHDF_MAX_NATTR_VAL];      /*Make sure it is long enough*/
	char cbuf[MYHDF_MAX_NATTR_VAL];
	char *hdfeos_version = "HDFEOS_V2.4";
	int ip, ic;
	int32 hdf_id;
	int32 vgroup_id[3];
	int32 sds_index, sds_id;
	char *grid_name = "Grid"; /* A silly name */
	char message[MSGLEN];

	char status = 0;
	int32 xdimsize;
	int32 ydimsize;

	int k;
	int32 rank = 2;
	int32 dim_sizes[2];
	int32 projCode = 1;                         // UTM, never use?????????????????????????
	int32 sphereCode=12;                        //WGS 84
	char  datum[] = "WGS84";		     // 
	int32 zoneCode = -1;                        //For UTM only. 
	char cproj[] = "GCTP_UTM";
	double pixsize[] = {10, 20, 60};
        int32 imgdim[] = {10980, 5490, 1830};
	double pixsz;
	int igrid;
	int datafield;	/* 1-based */

	float64 upleftpt[2], lowrightpt[2];
	float64 f_integral, f_fractional;
	int isds;

	/*
	float64 projParameters[NPROJ_PARAM_HDFEOS];
	for (k = 0; k < NPROJ_PARAM_HDFEOS; k++)
		projParameters[k] = 0.0;
	*/

	/*  Start to generate the character string for the global attribute:
	    #define SPACE_STRUCT_METADATA ("StructMetadata.0")
	*/

	ic = 0;
	sprintf(cbuf,
	    "GROUP=SwathStructure\n"
	    "END_GROUP=SwathStructure\n"
	    "GROUP=GridStructure\n");

	if (!AppendMeta(struct_meta, &ic, cbuf)) {
		Error("Error in AppendMeta() for (head)");
		return(1);
	}

	datafield = 0;
	upleftpt[0] = sds[0].ulx; /* Same for all SDS */
	upleftpt[1] = sds[0].uly;
	lowrightpt[0] = upleftpt[0] + sds[0].ncol * sds[0].pixsz;
	lowrightpt[1] = upleftpt[1] - sds[0].nrow * sds[0].pixsz;

	sprintf(cbuf, "\tGROUP=GRID\n");
	if (!AppendMeta(struct_meta, &ic, cbuf)) 
        {
	  Error("Error in AppendMeta() for (head)");
	  return(1);
	}

	sprintf(cbuf,
	  "\t\tGridName=\"%s\"\n",
	  grid_name);
	if (!AppendMeta(struct_meta, &ic, cbuf))
        {
 	  Error("Error in AppendMeta() for (head)");
	  return(1);
	}

	sprintf(cbuf,
          "\t\tGROUP=Dimension\n"
          "\t\t\tOBJECT=Dimension_1\n"
          "\t\t\t\tDimensionName=\"YDim_Grid_10m\"\n"
          "\t\t\t\tSize=%d\n"
          "\t\t\tEND_OBJECT=Dimension_1\n"
          "\t\t\tOBJECT=Dimension_2\n"
          "\t\t\t\tDimensionName=\"XDim_Grid_10m\"\n"
          "\t\t\t\tSize=%d\n"
          "\t\t\tEND_OBJECT=Dimension_2\n"
          "\t\t\tOBJECT=Dimension_3\n"
          "\t\t\t\tDimensionName=\"YDim_Grid_20m\"\n"
          "\t\t\t\tSize=%d\n"
          "\t\t\tEND_OBJECT=Dimension_3\n"
          "\t\t\tOBJECT=Dimension_4\n"
          "\t\t\t\tDimensionName=\"XDim_Grid_20m\"\n"
          "\t\t\t\tSize=%d\n"
          "\t\t\tEND_OBJECT=Dimension_4\n"
          "\t\t\tOBJECT=Dimension_5\n"
          "\t\t\t\tDimensionName=\"YDim_Grid_60m\"\n"
          "\t\t\t\tSize=%d\n"
          "\t\t\tEND_OBJECT=Dimension_5\n"
          "\t\t\tOBJECT=Dimension_6\n"
          "\t\t\t\tDimensionName=\"XDim_Grid_60m\"\n"
          "\t\t\t\tSize=%d\n"
          "\t\t\tEND_OBJECT=Dimension_6\n"
          "\t\tEND_GROUP=Dimension\n"
          "\t\tGROUP=DimensionMap\n"
          "\t\tEND_GROUP=DimensionMap\n"
          "\t\tGROUP=IndexDimensionMap\n"
          "\t\tEND_GROUP=IndexDimensionMap\n",
	imgdim[0], imgdim[0], imgdim[1], imgdim[1], imgdim[2], imgdim[2]);
	
	if (!AppendMeta(struct_meta, &ic, cbuf))
        {
	  Error("Error in AppendMeta() for (SDS group start)");
          return(1);
	}

        sprintf(cbuf,
          "\t\tGROUP=DataField\n");

        if (!AppendMeta(struct_meta, &ic, cbuf))
        {
	  Error("Error in AppendMeta() for (SDS group start)");
          return(1);
	}


	/* Put SDS group */

	/* All the SDS:
		"B01", "B02", "B03", "B04", "B05", "B06", 
	       	"B07", "B08", "B8A", "B09", "B10", "B11", "B12", 
		"ACmask", "Fmask"
	*/
	for (isds = 0; isds < nsds; isds++)
                {
	  if ( isds == 1 || isds == 2 || isds == 3 || isds == 7 || isds == 13)
                  {
            xdimsize = imgdim[0];
                    ydimsize = imgdim[0];
   	    pixsz = pixsize[0]; 
                  }
	  if ( isds == 4 || isds == 5 || isds == 6 || isds == 8 || isds == 11 || isds == 12)
                  {
            xdimsize = imgdim[1];
                    ydimsize = imgdim[1];
	    pixsz = pixsize[1]; 
                  }
	  if ( isds == 0 || isds == 9 || isds == 10)
                  {
            xdimsize = imgdim[2];
                    ydimsize = imgdim[2];
	    pixsz = pixsize[2]; 
                  }

 		  datafield++;

	  sprintf(cbuf,
	    "\t\t\tXDim=%d\n"
	    "\t\t\tYDim=%d\n"
	    "\t\t\tPixelSize=(%.1lf,%.1lf)\n",
	    xdimsize, ydimsize,
	    pixsz, pixsz);

	  if (!AppendMeta(struct_meta, &ic, cbuf)) 
                  {
		Error("Error in AppendMeta() for (grid information start)");
		return(1);
	  }

                  sprintf(cbuf,
	    "\t\tUpperLeftPointMtrs=(%.6lf,%.6lf)\n"
	    "\t\tLowerRightMtrs=(%.6lf,%.6lf)\n"
	    "\t\tProjection=%s\n",
	    upleftpt[0],   upleftpt[1],
	    lowrightpt[0], lowrightpt[1],
	    cproj);

                  if (!AppendMeta(struct_meta, &ic, cbuf))
                  {
            Error("Error in AppendMeta() for (grid information start)");
	    return(1);
          }

          sprintf(cbuf,
	    "\t\tZoneCode=%d\n"
	    "\t\tSphereCode=%d\n"
	    "\t\tDatum=%s\n"
	    "\t\tGridOrigin=HDFE_GD_UL\n",
	    sds[0].zonecode,			// Same for all SDS
	    sphereCode,
	    datum);
                  if (!AppendMeta(struct_meta, &ic, cbuf))
                  {
                    Error("Error in AppendMeta() for (grid information end)");
	    return(1);
          }

	  sprintf(cbuf,
	    "\t\t\tOBJECT=DataField_%d\n"
        	    "\t\t\t\tDataFieldName=\"%s\"\n"
            "\t\t\t\tDataType=%s\n"
	    "\t\t\t\tDimList=(\"%s\",\"%s\")\n"
	    "\t\t\tEND_OBJECT=DataField_%d\n",
	  datafield, sds[isds].name, sds[isds].data_type_name, sds[isds].dimname[0], sds[isds].dimname[1], datafield);
	  if (!AppendMeta(struct_meta, &ic, cbuf))
                  {
	    Error("Error in AppendMeta() for (SDS group)");
	    return(1);
	  }
        }

	sprintf(cbuf,
	    "\t\tEND_GROUP=DataField\n"
	    "\t\tGROUP=MergedFields\n"
	    "\t\tEND_GROUP=MergedFields\n");

	if (!AppendMeta(struct_meta, &ic, cbuf)) {
	    	Error("Error in AppendMeta() for (SDS group end)");
		return(1);
	}

	/* Put trailer */
	sprintf(cbuf,
    		"\tEND_GROUP=GRID\n");
	if (!AppendMeta(struct_meta, &ic, cbuf)) {
    		Error("Error in AppendMeta() for (tail)");
		return(1);
	}

	sprintf(cbuf,
	    "END_GROUP=GridStructure\n"
	    "GROUP=PointStructure\n"
	    "END_GROUP=PointStructure\n"
	    "END\n");

	if (!AppendMeta(struct_meta, &ic, cbuf)) {
	    	Error("Error in AppendMeta() for (tail)");
		return(1);
	}


	if ( PutSpaceDefHDF(hdfname, struct_meta, sds, nsds) != 0)
		exit(1);

	return(0);
}


int set_S30_sds_info(sds_info_t *all_sds,  int nsds,  s2at30m_t *s2r)
{
	int iband;
	double pixsz = 30.0;	
	char message[MSGLEN];

	char *dimnames[2] =  {"YDim_Grid", "XDim_Grid"};

	if (nsds != S2NBAND+2) {
		sprintf(message, "Number of spectral bands + 2 masks must be %d", S2NBAND+2);
		Error(message);
		return(1);
	}

	/* Spectral bands */
	for (iband = 0; iband < S2NBAND; iband++) {
		strcpy(all_sds[iband].name,  S2_SDS_NAME[iband]);
		strcpy(all_sds[iband].data_type_name, "DFNT_INT16");
		all_sds[iband].data_type = DFNT_INT16;
		strcpy(all_sds[iband].dimname[0], dimnames[0]);
		strcpy(all_sds[iband].dimname[1], dimnames[1]);

		all_sds[iband].ulx = s2r->ulx;
		all_sds[iband].uly = s2r->uly;
		all_sds[iband].nrow = s2r->nrow;
		all_sds[iband].ncol = s2r->ncol;
		all_sds[iband].pixsz = pixsz;
		all_sds[iband].zonecode =  atoi(s2r->zonehem);
	}

	/* ACmask */
	strcpy(all_sds[nsds-2].name,  ACMASK_NAME);
	strcpy(all_sds[nsds-2].data_type_name, "DFNT_UINT8");
	all_sds[nsds-2].data_type = DFNT_UINT8;
	strcpy(all_sds[nsds-2].dimname[0], dimnames[0]);
	strcpy(all_sds[nsds-2].dimname[1], dimnames[1]);

	/* Fmask */
	strcpy(all_sds[nsds-1].name,  FMASK_NAME);
	strcpy(all_sds[nsds-1].data_type_name, "DFNT_UINT8");
	all_sds[nsds-1].data_type = DFNT_UINT8;
	strcpy(all_sds[nsds-1].dimname[0], dimnames[0]);
	strcpy(all_sds[nsds-1].dimname[1], dimnames[1]);


	return(0);
}

int S30_PutSpaceDefHDF(char *hdfname, sds_info_t sds[], int nsds)
{
	int32 sd_id;
	char struct_meta[MYHDF_MAX_NATTR_VAL];      /*Make sure it is long enough*/
	char cbuf[MYHDF_MAX_NATTR_VAL];
	char *hdfeos_version = "HDFEOS_V2.4";
	int ip, ic;
	int32 hdf_id;
	int32 vgroup_id[3];
	int32 sds_index, sds_id;
	char *grid_name = "Grid"; /* A silly name */
	char message[MSGLEN];

	char status = 0;
	int32 xdimsize;
	int32 ydimsize;

	int k;
	int32 rank = 2;
	int32 dim_sizes[2];
	int32 projCode = 1;                         // UTM, never use?????????????????????????
	int32 sphereCode=12;                        //WGS 84
	char  datum[] = "WGS84";		     // 
	int32 zoneCode = -1;                        //For UTM only. 
	char cproj[] = "GCTP_UTM";
	int datafield;	/* Does data field ID reset for each GRID, or it is cumulative? Oct 19, 2016. */
	double pixsz;

	float64 upleftpt[2], lowrightpt[2];
	float64 f_integral, f_fractional;
	int isds;

	/*
	float64 projParameters[NPROJ_PARAM_HDFEOS];
	for (k = 0; k < NPROJ_PARAM_HDFEOS; k++)
		projParameters[k] = 0.0;
	*/

	/*  Start to generate the character string for the global attribute:
	    #define SPACE_STRUCT_METADATA ("StructMetadata.0")
	*/

	ic = 0;
	sprintf(cbuf,
	    "GROUP=SwathStructure\n"
	    "END_GROUP=SwathStructure\n"
	    "GROUP=GridStructure\n");

	if (!AppendMeta(struct_meta, &ic, cbuf)) {
		Error("Error in AppendMeta() for (head)");
		return(1);
	}

	sprintf(cbuf,
    	"\tGROUP=GRID_1\n");

	if (!AppendMeta(struct_meta, &ic, cbuf)) {
		Error("Error in AppendMeta() for (head)");
		return(1);
	}

	/* Put Grid information.Same for all SDS */
	xdimsize = sds[0].ncol;
	ydimsize = sds[0].nrow;
	pixsz = sds[0].pixsz;
	upleftpt[0] = sds[0].ulx; /* Same for all SDS */
	upleftpt[1] = sds[0].uly;
	lowrightpt[0] = upleftpt[0] + sds[0].ncol * sds[0].pixsz;
	lowrightpt[1] = upleftpt[1] - sds[0].nrow * sds[0].pixsz;

	sprintf(cbuf,
	    "\t\tGridName=\"%s\"\n"
	    "\t\tXDim=%d\n"
	    "\t\tYDim=%d\n"
	    "\t\tPixelSize=(%.1lf,%.1lf)\n"
	    "\t\tUpperLeftPointMtrs=(%.6lf,%.6lf)\n"
	    "\t\tLowerRightMtrs=(%.6lf,%.6lf)\n"
	    "\t\tProjection=%s\n",
	    grid_name,
	    xdimsize, ydimsize,
	    pixsz, pixsz,
	    upleftpt[0],   upleftpt[1],
	    lowrightpt[0], lowrightpt[1],
	    cproj);

	if (!AppendMeta(struct_meta, &ic, cbuf)) {
		Error("Error in AppendMeta() for (grid information start)");
		return(1);
	}


	sprintf(cbuf,
	    "\t\tZoneCode=%d\n"
	    "\t\tSphereCode=%d\n"
	    "\t\tDatum=%s\n"
	    "\t\tGridOrigin=HDFE_GD_UL\n",
	    sds[0].zonecode,			/* Same for all SDS */
	    sphereCode,
	    datum);
	if (!AppendMeta(struct_meta, &ic, cbuf)) {
		Error("Error in AppendMeta() for (grid information end)");
		return(1);
	}

	/* Put SDS group */
	sprintf(cbuf,
	    "\t\tGROUP=Dimension\n"
                        "\t\t\tOBJECT=Dimension_1\n"
                                "\t\t\t\tDimensionName=\"YDim_Grid\"\n"
                                "\t\t\t\tSize=%d\n"
                        "\t\t\tEND_OBJECT=Dimension_1\n"
                        "\t\t\tOBJECT=Dimension_2\n"
                                "\t\t\t\tDimensionName=\"XDim_Grid\"\n"
                                "\t\t\t\tSize=%d\n"
                        "\t\t\tEND_OBJECT=Dimension_2\n"
	    "\t\tEND_GROUP=Dimension\n"
            "\t\tGROUP=DimensionMap\n"
            "\t\tEND_GROUP=DimensionMap\n"
            "\t\tGROUP=IndexDimensionMap\n"
            "\t\tEND_GROUP=IndexDimensionMap\n"
	    "\t\tGROUP=DataField\n", ydimsize, xdimsize);

	if (!AppendMeta(struct_meta, &ic, cbuf)) {
		Error("Error in AppendMeta() for (SDS group start)");
		return(1);
	}

	/* All the SDS:
		"B01", "B02", "B03", "B04", "B05", "B06", 
	       	"B07", "B08", "B8A", "B09", "B10", "B11", "B12", 
		"QA"
	*/
	datafield = 0;
	for (isds = 0; isds < nsds; isds++) {
		datafield++;
		sprintf(cbuf,
			"\t\t\tOBJECT=DataField_%d\n"
			"\t\t\t\tDataFieldName=\"%s\"\n"
			"\t\t\t\tDataType=%s\n"
			"\t\t\t\tDimList=(\"%s\",\"%s\")\n"
			"\t\t\tEND_OBJECT=DataField_%d\n",
			datafield, sds[isds].name, sds[isds].data_type_name, sds[isds].dimname[0], sds[isds].dimname[1], datafield);
		if (!AppendMeta(struct_meta, &ic, cbuf)) {
			Error("Error in AppendMeta() for (SDS group)");
			return(1);
		}

	}

	sprintf(cbuf,
	    "\t\tEND_GROUP=DataField\n"
	    "\t\tGROUP=MergedFields\n"
	    "\t\tEND_GROUP=MergedFields\n");

	if (!AppendMeta(struct_meta, &ic, cbuf)) {
	    	Error("Error in AppendMeta() for (SDS group end)");
		return(1);
	}

	/* Put trailer */
	sprintf(cbuf,
    		"\tEND_GROUP=GRID_1\n");
	if (!AppendMeta(struct_meta, &ic, cbuf)) {
    		Error("Error in AppendMeta() for (tail)");
		return(1);
	}

	sprintf(cbuf,
	    "END_GROUP=GridStructure\n"
	    "GROUP=PointStructure\n"
	    "END_GROUP=PointStructure\n"
	    "END\n");

	if (!AppendMeta(struct_meta, &ic, cbuf)) {
	    	Error("Error in AppendMeta() for (tail)");
		return(1);
	}

	if ( PutSpaceDefHDF(hdfname, struct_meta, sds, nsds) != 0)
		exit(1);

	return(0);
}


int set_L30_sds_info(sds_info_t *all_sds,  int nsds,  lsat_t *lsat)
{
	int iband;
	double pixsz = 30.0;	
	char message[MSGLEN];

	char *dimnames[2] =  {"YDim_Grid", "XDim_Grid"};

	if (nsds != L8NRB+L8NTB+2) {
		sprintf(message, "Number of spectral bands + 2 masks must be %d", L8NRB+L8NTB+2);
		Error(message);
		return(1);
	}

	/* Reflective bands */
	int nb = 0;
	for (iband = 0; iband < L8NRB; iband++) {
		strcpy(all_sds[nb].name,  L8_REF_SDS_NAME[1][iband]);
		strcpy(all_sds[nb].data_type_name, "DFNT_INT16");
		all_sds[nb].data_type = DFNT_INT16;
		strcpy(all_sds[nb].dimname[0], dimnames[0]);
		strcpy(all_sds[nb].dimname[1], dimnames[1]);

		all_sds[nb].ulx = lsat->ulx;
		all_sds[nb].uly = lsat->uly;
		all_sds[nb].nrow = lsat->nrow;
		all_sds[nb].ncol = lsat->ncol;
		all_sds[nb].pixsz = pixsz;
		all_sds[nb].zonecode =  atoi(lsat->zonehem);
	
		nb++;
	}

	/* thermal bands */
	for (iband = 0; iband < L8NTB; iband++) {
		strcpy(all_sds[nb].name,  L8_THM_SDS_NAME[1][iband]);
		strcpy(all_sds[nb].data_type_name, "DFNT_INT16");
		all_sds[nb].data_type = DFNT_INT16;
		strcpy(all_sds[nb].dimname[0], dimnames[0]);
		strcpy(all_sds[nb].dimname[1], dimnames[1]);

		all_sds[nb].ulx = lsat->ulx;
		all_sds[nb].uly = lsat->uly;
		all_sds[nb].nrow = lsat->nrow;
		all_sds[nb].ncol = lsat->ncol;
		all_sds[nb].pixsz = pixsz;
		all_sds[nb].zonecode =  atoi(lsat->zonehem);

		nb++;
	}

	/* ACmask */
	strcpy(all_sds[nsds-2].name,  ACMASK_NAME);
	strcpy(all_sds[nsds-2].data_type_name, "DFNT_UINT8");
	all_sds[nsds-2].data_type = DFNT_UINT8;
	strcpy(all_sds[nsds-2].dimname[0], dimnames[0]);
	strcpy(all_sds[nsds-2].dimname[1], dimnames[1]);

	/* Fmask */
	strcpy(all_sds[nsds-1].name,  FMASK_NAME);
	strcpy(all_sds[nsds-1].data_type_name, "DFNT_UINT8");
	all_sds[nsds-1].data_type = DFNT_UINT8;
	strcpy(all_sds[nsds-1].dimname[0], dimnames[0]);
	strcpy(all_sds[nsds-1].dimname[1], dimnames[1]);


	return(0);
}

int L30_PutSpaceDefHDF(char *hdfname, sds_info_t sds[], int nsds)
{
	int32 sd_id;
	char struct_meta[MYHDF_MAX_NATTR_VAL];      /*Make sure it is long enough*/
	char cbuf[MYHDF_MAX_NATTR_VAL];
	char *hdfeos_version = "HDFEOS_V2.4";
	int ip, ic;
	int32 hdf_id;
	int32 vgroup_id[3];
	int32 sds_index, sds_id;
	char *grid_name = "Grid"; /* A silly name */
	char message[MSGLEN];

	char status = 0;
	int32 xdimsize;
	int32 ydimsize;

	int k;
	int32 rank = 2;
	int32 dim_sizes[2];
	int32 projCode = 1;                         // UTM, never use?????????????????????????
	int32 sphereCode=12;                        //WGS 84
	char  datum[] = "WGS84";		     // 
	int32 zoneCode = -1;                        //For UTM only. 
	char cproj[] = "GCTP_UTM";
	int datafield;	/* Does data field ID reset for each GRID, or it is cumulative? Oct 19, 2016. */
	double pixsz;

	float64 upleftpt[2], lowrightpt[2];
	float64 f_integral, f_fractional;
	int isds;

	/*
	float64 projParameters[NPROJ_PARAM_HDFEOS];
	for (k = 0; k < NPROJ_PARAM_HDFEOS; k++)
		projParameters[k] = 0.0;
	*/

	/*  Start to generate the character string for the global attribute:
	    #define SPACE_STRUCT_METADATA ("StructMetadata.0")
	*/

	ic = 0;
	sprintf(cbuf,
	    "GROUP=SwathStructure\n"
	    "END_GROUP=SwathStructure\n"
	    "GROUP=GridStructure\n");

	if (!AppendMeta(struct_meta, &ic, cbuf)) {
		Error("Error in AppendMeta() for (head)");
		return(1);
	}

	sprintf(cbuf,
    	"\tGROUP=GRID_1\n");

	if (!AppendMeta(struct_meta, &ic, cbuf)) {
		Error("Error in AppendMeta() for (head)");
		return(1);
	}

	/* Put Grid information.Same for all SDS */
	xdimsize = sds[0].ncol;
	ydimsize = sds[0].nrow;
	pixsz = sds[0].pixsz;
	upleftpt[0] = sds[0].ulx; /* Same for all SDS */
	upleftpt[1] = sds[0].uly;
	lowrightpt[0] = upleftpt[0] + sds[0].ncol * sds[0].pixsz;
	lowrightpt[1] = upleftpt[1] - sds[0].nrow * sds[0].pixsz;

	sprintf(cbuf,
	    "\t\tGridName=\"%s\"\n"
	    "\t\tXDim=%d\n"
	    "\t\tYDim=%d\n"
	    "\t\tPixelSize=(%.1lf,%.1lf)\n"
	    "\t\tUpperLeftPointMtrs=(%.6lf,%.6lf)\n"
	    "\t\tLowerRightMtrs=(%.6lf,%.6lf)\n"
	    "\t\tProjection=%s\n",
	    grid_name,
	    xdimsize, ydimsize,
	    pixsz, pixsz,
	    upleftpt[0],   upleftpt[1],
	    lowrightpt[0], lowrightpt[1],
	    cproj);

	if (!AppendMeta(struct_meta, &ic, cbuf)) {
		Error("Error in AppendMeta() for (grid information start)");
		return(1);
	}


	sprintf(cbuf,
	    "\t\tZoneCode=%d\n"
	    "\t\tSphereCode=%d\n"
	    "\t\tDatum=%s\n"
	    "\t\tGridOrigin=HDFE_GD_UL\n",
	    sds[0].zonecode,			/* Same for all SDS */
	    sphereCode,
	    datum);
	if (!AppendMeta(struct_meta, &ic, cbuf)) {
		Error("Error in AppendMeta() for (grid information end)");
		return(1);
	}

	/* Put SDS group */
	sprintf(cbuf,
	    "\t\tGROUP=Dimension\n"
                        "\t\t\tOBJECT=Dimension_1\n"
                                "\t\t\t\tDimensionName=\"YDim_Grid\"\n"
                                "\t\t\t\tSize=%d\n"
                        "\t\t\tEND_OBJECT=Dimension_1\n"
                        "\t\t\tOBJECT=Dimension_2\n"
                                "\t\t\t\tDimensionName=\"XDim_Grid\"\n"
                                "\t\t\t\tSize=%d\n"
                        "\t\t\tEND_OBJECT=Dimension_2\n"
	    "\t\tEND_GROUP=Dimension\n"
            "\t\tGROUP=DimensionMap\n"
            "\t\tEND_GROUP=DimensionMap\n"
            "\t\tGROUP=IndexDimensionMap\n"
            "\t\tEND_GROUP=IndexDimensionMap\n"
	    "\t\tGROUP=DataField\n", ydimsize, xdimsize);

	if (!AppendMeta(struct_meta, &ic, cbuf)) {
		Error("Error in AppendMeta() for (SDS group start)");
		return(1);
	}

	/* All the SDS including the 2 masks
	*/
	datafield = 0;
	for (isds = 0; isds < nsds; isds++) {
		datafield++;
		sprintf(cbuf,
			"\t\t\tOBJECT=DataField_%d\n"
			"\t\t\t\tDataFieldName=\"%s\"\n"
			"\t\t\t\tDataType=%s\n"
			"\t\t\t\tDimList=(\"%s\",\"%s\")\n"
			"\t\t\tEND_OBJECT=DataField_%d\n",
			datafield, sds[isds].name, sds[isds].data_type_name, sds[isds].dimname[0], sds[isds].dimname[1], datafield);
		if (!AppendMeta(struct_meta, &ic, cbuf)) {
			Error("Error in AppendMeta() for (SDS group)");
			return(1);
		}

	}

	sprintf(cbuf,
	    "\t\tEND_GROUP=DataField\n"
	    "\t\tGROUP=MergedFields\n"
	    "\t\tEND_GROUP=MergedFields\n");

	if (!AppendMeta(struct_meta, &ic, cbuf)) {
	    	Error("Error in AppendMeta() for (SDS group end)");
		return(1);
	}

	/* Put trailer */
	sprintf(cbuf,
    		"\tEND_GROUP=GRID_1\n");
	if (!AppendMeta(struct_meta, &ic, cbuf)) {
    		Error("Error in AppendMeta() for (tail)");
		return(1);
	}

	sprintf(cbuf,
	    "END_GROUP=GridStructure\n"
	    "GROUP=PointStructure\n"
	    "END_GROUP=PointStructure\n"
	    "END\n");

	if (!AppendMeta(struct_meta, &ic, cbuf)) {
	    	Error("Error in AppendMeta() for (tail)");
		return(1);
	}

	if ( PutSpaceDefHDF(hdfname, struct_meta, sds, nsds) != 0)
		exit(1);

	return(0);
}

int set_S2ang_sds_info(sds_info_t *all_sds,  int nsds,  s2ang_t *s2ang)
{
	int iband;
	char message[MSGLEN];
	int NBAND = 4;

	char *dimnames[2] =  {"YDim_Grid", "XDim_Grid"};

	if (nsds != NBAND) {
		sprintf(message, "Number of angle bands must be %d", NBAND);
		Error(message);
		return(1);
	}

	for (iband = 0; iband < NBAND; iband++) {
		strcpy(all_sds[iband].name,  ANG_SDS_NAME[iband]);
		strcpy(all_sds[iband].data_type_name, "DFNT_UINT16");
		all_sds[iband].data_type = DFNT_UINT16;
		strcpy(all_sds[iband].dimname[0], dimnames[0]);
		strcpy(all_sds[iband].dimname[1], dimnames[1]);

		all_sds[iband].ulx = s2ang->ulx;
		all_sds[iband].uly = s2ang->uly;
		all_sds[iband].nrow = s2ang->nrow;
		all_sds[iband].ncol = s2ang->ncol;
		all_sds[iband].pixsz = ANGPIXSZ; 
		all_sds[iband].zonecode =  atoi(s2ang->zonehem);
	}

	return(0);
}

int set_L8ang_sds_info(sds_info_t *all_sds,  int nsds,  l8ang_t *l8ang)
{
	int iband;
	char message[MSGLEN];
	int NBAND = 4;

	char *dimnames[2] =  {"YDim_Grid", "XDim_Grid"};

	if (nsds != NBAND) {
		sprintf(message, "Number of angle bands must be %d", NBAND);
		Error(message);
		return(1);
	}

	for (iband = 0; iband < NBAND; iband++) {
		strcpy(all_sds[iband].name,  ANG_SDS_NAME[iband]);
		strcpy(all_sds[iband].data_type_name, "DFNT_UINT16");
		all_sds[iband].data_type = DFNT_UINT16;
		strcpy(all_sds[iband].dimname[0], dimnames[0]);
		strcpy(all_sds[iband].dimname[1], dimnames[1]);

		all_sds[iband].ulx = l8ang->ulx;
		all_sds[iband].uly = l8ang->uly;
		all_sds[iband].nrow = l8ang->nrow;
		all_sds[iband].ncol = l8ang->ncol;
		all_sds[iband].pixsz = ANGPIXSZ; 
		all_sds[iband].zonecode =  atoi(l8ang->zonehem);
	}

	return(0);
}

//int set_aod_sds_info(sds_info_t *all_sds,  int nsds, aod_t *aod)
//{
//	char message[MSGLEN];
//	char *dimnames[2] =  {"YDim_Grid", "XDim_Grid"};
//	int iband = 0; 	/* There is only one SDS */
//
//	if (nsds != 1) {
//		fprintf(stderr, "Only 1 SDS allowed in set_aod_sds_info()\n");
//		return(1);
//	}	
//	iband = 0;
//
//	strcpy(all_sds[iband].name,  LASRC_AOD);
//	strcpy(all_sds[iband].data_type_name, "DFNT_UINT8");
//	all_sds[iband].data_type = DFNT_UINT8;
//	strcpy(all_sds[iband].dimname[0], dimnames[0]);
//	strcpy(all_sds[iband].dimname[1], dimnames[1]);
//
//	all_sds[iband].ulx = aod->ulx;
//	all_sds[iband].uly = aod->uly;
//	all_sds[iband].nrow = aod->nrow;
//	all_sds[iband].ncol = aod->ncol;
//	all_sds[iband].pixsz = HLS_PIXSZ; 
//	all_sds[iband].zonecode =  atoi(aod->zonehem);
//
//	return(0);
//}

int angle_PutSpaceDefHDF(char *hdfname, sds_info_t sds[], int nsds)
{
	int32 sd_id;
	char struct_meta[MYHDF_MAX_NATTR_VAL];      /*Make sure it is long enough*/
	char cbuf[MYHDF_MAX_NATTR_VAL];
	char *hdfeos_version = "HDFEOS_V2.4";
	int ip, ic;
	int32 sds_index, sds_id;
	char *grid_name = "Grid"; /* A silly name */
	char message[MSGLEN];

	char status = 0;
	int32 xdimsize;
	int32 ydimsize;

	int k;
	int32 rank = 2;
	int32 dim_sizes[2];
	int32 projCode = 1;                         // UTM, never use?????????????????????????
	int32 sphereCode=12;                        //WGS 84
	char  datum[] = "WGS84";		     // 
	int32 zoneCode = -1;                        //For UTM only. 
	char cproj[] = "GCTP_UTM";
	int datafield;	/* Does data field ID reset for each GRID, or it is cumulative? Oct 19, 2016. */
	double pixsz;

	float64 upleftpt[2], lowrightpt[2];
	float64 f_integral, f_fractional;
	int isds;

	/*
	float64 projParameters[NPROJ_PARAM_HDFEOS];
	for (k = 0; k < NPROJ_PARAM_HDFEOS; k++)
		projParameters[k] = 0.0;
	*/

	/*  Start to generate the character string for the global attribute:
	    #define SPACE_STRUCT_METADATA ("StructMetadata.0")
	*/

	ic = 0;
	sprintf(cbuf,
	    "GROUP=SwathStructure\n"
	    "END_GROUP=SwathStructure\n"
	    "GROUP=GridStructure\n");

	if (!AppendMeta(struct_meta, &ic, cbuf)) {
		Error("Error in AppendMeta() for (head)");
		return(1);
	}

	sprintf(cbuf,
    	"\tGROUP=GRID_1\n");

	if (!AppendMeta(struct_meta, &ic, cbuf)) {
		Error("Error in AppendMeta() for (head)");
		return(1);
	}

	/* Put Grid information.Same for all SDS */
	xdimsize = sds[0].ncol;
	ydimsize = sds[0].nrow;
	pixsz = sds[0].pixsz;
	upleftpt[0] = sds[0].ulx; /* Same for all SDS */
	upleftpt[1] = sds[0].uly;
	lowrightpt[0] = upleftpt[0] + sds[0].ncol * sds[0].pixsz;
	lowrightpt[1] = upleftpt[1] - sds[0].nrow * sds[0].pixsz;

	sprintf(cbuf,
	    "\t\tGridName=\"%s\"\n"
	    "\t\tXDim=%d\n"
	    "\t\tYDim=%d\n"
	    "\t\tPixelSize=(%.1lf,%.1lf)\n"
	    "\t\tUpperLeftPointMtrs=(%.6lf,%.6lf)\n"
	    "\t\tLowerRightMtrs=(%.6lf,%.6lf)\n"
	    "\t\tProjection=%s\n",
	    grid_name,
	    xdimsize, ydimsize,
	    pixsz, pixsz,
	    upleftpt[0],   upleftpt[1],
	    lowrightpt[0], lowrightpt[1],
	    cproj);

	if (!AppendMeta(struct_meta, &ic, cbuf)) {
		Error("Error in AppendMeta() for (grid information start)");
		return(1);
	}

	sprintf(cbuf,
	    "\t\tZoneCode=%d\n"
	    "\t\tSphereCode=%d\n"
	    "\t\tDatum=%s\n"
	    "\t\tGridOrigin=HDFE_GD_UL\n",
	    sds[0].zonecode,			/* Same for all SDS */
	    sphereCode,
	    datum);
	if (!AppendMeta(struct_meta, &ic, cbuf)) {
		Error("Error in AppendMeta() for (grid information end)");
		return(1);
	}

	/* Put SDS group */
	sprintf(cbuf,
	    "\t\tGROUP=Dimension\n"
                        "\t\t\tOBJECT=Dimension_1\n"
                                "\t\t\t\tDimensionName=\"YDim_Grid\"\n"
                                "\t\t\t\tSize=%d\n"
                        "\t\t\tEND_OBJECT=Dimension_1\n"
                        "\t\t\tOBJECT=Dimension_2\n"
                                "\t\t\t\tDimensionName=\"XDim_Grid\"\n"
                                "\t\t\t\tSize=%d\n"
                        "\t\t\tEND_OBJECT=Dimension_2\n"
	    "\t\tEND_GROUP=Dimension\n"
            "\t\tGROUP=DimensionMap\n"
            "\t\tEND_GROUP=DimensionMap\n"
            "\t\tGROUP=IndexDimensionMap\n"
            "\t\tEND_GROUP=IndexDimensionMap\n"
	    "\t\tGROUP=DataField\n", ydimsize, xdimsize);

	if (!AppendMeta(struct_meta, &ic, cbuf)) {
		Error("Error in AppendMeta() for (SDS group start)");
		return(1);
	}

	datafield = 0;
	for (isds = 0; isds < nsds; isds++) {
		datafield++;
		sprintf(cbuf,
			"\t\t\tOBJECT=DataField_%d\n"
			"\t\t\t\tDataFieldName=\"%s\"\n"
			"\t\t\t\tDataType=%s\n"
			"\t\t\t\tDimList=(\"%s\",\"%s\")\n"
			"\t\t\tEND_OBJECT=DataField_%d\n",
			datafield, sds[isds].name, sds[isds].data_type_name, sds[isds].dimname[0], sds[isds].dimname[1], datafield);
		if (!AppendMeta(struct_meta, &ic, cbuf)) {
			Error("Error in AppendMeta() for (SDS group)");
			return(1);
		}

	}

	sprintf(cbuf,
	    "\t\tEND_GROUP=DataField\n"
	    "\t\tGROUP=MergedFields\n"
	    "\t\tEND_GROUP=MergedFields\n");

	if (!AppendMeta(struct_meta, &ic, cbuf)) {
	    	Error("Error in AppendMeta() for (SDS group end)");
		return(1);
	}

	/* Put trailer */
	sprintf(cbuf,
    		"\tEND_GROUP=GRID_1\n");
	if (!AppendMeta(struct_meta, &ic, cbuf)) {
    		Error("Error in AppendMeta() for (tail)");
		return(1);
	}

	sprintf(cbuf,
	    "END_GROUP=GridStructure\n"
	    "GROUP=PointStructure\n"
	    "END_GROUP=PointStructure\n"
	    "END\n");

	if (!AppendMeta(struct_meta, &ic, cbuf)) {
	    	Error("Error in AppendMeta() for (tail)");
		return(1);
	}

	
	if (PutSpaceDefHDF(hdfname, struct_meta, sds, nsds) != 0)
		exit(0);

	return(0);
}

int PutSpaceDefHDF(char *hdfname, char *struct_meta, sds_info_t sds[], int nsds)
{

	int32 sd_id;
	int32 hdf_id;
	int32 sds_index, sds_id, isds;
	int32 vgroup_id[3];
	char *grid_name = "Grid"; /* A silly name */
	char message[MSGLEN];

	/* Write the StructMetadata attribute to the HDF file.  
	 * Other global attributes have been set when the HDF is opened. */
	/* DFACC_RDWR is for update */
	if ((sd_id = SDstart(hdfname, DFACC_RDWR)) == FAIL) {
		sprintf(message, "Cannot open %s for DFACC_RDWR", hdfname);
		Error(message);
	} 
	
	if (SDsetattr(sd_id, SPACE_STRUCT_METADATA, DFNT_CHAR8, strlen(struct_meta), (VOIDP)struct_meta) == FAIL) {
		Error("Error write global attributes");
		return(1);
	}
	if (SDend(sd_id) == FAIL) {
	    	sprintf(message, "Error in SDend() for %s", hdfname);
	    	Error(message);
		return(1);
	}

	/* Setup the HDF Vgroup */
	if ((hdf_id = Hopen(hdfname, DFACC_RDWR, 0)) == FAIL) {
		sprintf(message, "Error in Hopen () for %s", hdfname);
	    	Error(message);
		return(1);
	}

	/* Start the Vgroup access */
	if (Vstart(hdf_id) == FAIL) {
	    	sprintf(message, "Error in Vstart () for %s", hdfname);
	    	Error(message);
		return(1);
	}

	/* Create Root Vgroup for Grid */
	vgroup_id[0] = Vattach(hdf_id, -1, "w");
	if (vgroup_id[0] == FAIL) {
		Error("Error in getting Grid Vgroup ID (Vattach)");
		return(1);
	}

	if (Vsetname(vgroup_id[0], grid_name) == FAIL) {
		Error("Error in setting Grid Vgroup name (Vsetname)");
		return(1);
	}

	if (Vsetclass(vgroup_id[0], "GRID") == FAIL) {
		Error("Error in setting Grid Vgroup class (Vsetclass)");
		return(1);
	}

	/* Create Data Fields Vgroup */
	vgroup_id[1] = Vattach(hdf_id, -1, "w");
	if (vgroup_id[1] == FAIL) {
		Error("Error in getting Data Fields Vgroup ID (Vattach)");
		return(1);
	}
	if (Vsetname(vgroup_id[1], "Data Fields") == FAIL) {
		Error("Error setting Data Fields Vgroup name (Vsetname)");
		return(1);
	}
	if (Vsetclass(vgroup_id[1], "GRID Vgroup") == FAIL) {
		Error("Error in setting Data Fields Vgroup class (Vsetclass)");
		return(1);
	}
	if (Vinsert(vgroup_id[0], vgroup_id[1]) == FAIL) {
		Error("Error in inserting Data Fields Vgroup (Vinsert)");
		return(1);
	}

	/* Create Attributes Vgroup */
	vgroup_id[2] = Vattach(hdf_id, -1, "w");
	if (vgroup_id[2] == FAIL) {
		Error("Error in getting Attributes Vgroup ID (Vattach)");
		return(1);
	}
	if (Vsetname(vgroup_id[2], "Grid Attributes") == FAIL) {
		Error("Error in setting Attributes Vgroup name (Vsetname)");
		return(1);
	}
	if (Vsetclass(vgroup_id[2], "GRID Vgroup") == FAIL) { 
	    	Error("Error in setting Attributes Vgroup class (Vsetclass)");
		return(1);
	}
	if (Vinsert(vgroup_id[0], vgroup_id[2]) == FAIL) {
		Error("Error in inserting Attributes Vgroup (Vinsert)");
		return(1);
	}

	/* Attach SDSs to Data Fields Vgroup */
	sd_id = SDstart(hdfname, DFACC_RDWR);
	if (sd_id == FAIL) {
	    	Error("Error in opening output file for DFACC_RDWR(2)");
		return(1);
	}
	for (isds = 0; isds < nsds; isds++) {
		sds_index = SDnametoindex(sd_id, sds[isds].name);
		if (sds_index == FAIL) {
			sprintf(message, "Error in getting SDS index (SDnametoindex) for %s", sds[isds].name); 
			Error(message);
			return(1);
		}
		sds_id = SDselect(sd_id, sds_index);
		if (sds_id == FAIL) {
			Error("Error in getting SDS ID (SDselect)");
			return(1);
		}
		if (Vaddtagref(vgroup_id[1], DFTAG_NDG, SDidtoref(sds_id)) == FAIL) {
			Error("Error in adding reference tag to SDS (Vaddtagref)");
			return(1);
		}
		if (SDendaccess(sds_id) == FAIL) {
			Error("Error in SDendaccess");
			return(1);
		}
	}
	if (SDend(sd_id) == FAIL) {
	    	Error("Error in SDend");
		return(1);
	}
	    

	/* Detach Vgroups */
	if (Vdetach(vgroup_id[0]) == FAIL) {
	    	Error("Error in detaching from Grid Vgroup (Vdetach)");
		return(1);
	}
	if (Vdetach(vgroup_id[1]) == FAIL) {
		Error("Error in detaching from Data Fields Vgroup (Vdetach)");
		return(1);
	}
	if (Vdetach(vgroup_id[2]) == FAIL) {
	    	Error("Error in detaching from Attributes Vgroup (Vdetach)");
		return(1);
	}

	/* Close access */
	if (Vend(hdf_id) == FAIL) { 
	    	Error("Error in end Vgroup access (Vend)");
		return(1);
	}
	if (Hclose(hdf_id) == FAIL) {
	    	Error("Error in end HDF access (Hclose)");
		return(1);
	}

	return(0);
}



/*********************************************************************************
    Append the characters in *s to *cbuf.

    Copied from LEDAPS space.c
*/
bool AppendMeta(char *cbuf, int *ic, char *s) {

	char message[MSGLEN];
	int nc, i;

	if (ic < 0) return false;
	nc = strlen(s);
	if (nc <= 0) return false;
	if (*ic + nc > MYHDF_MAX_NATTR_VAL) {
		sprintf(message, "Array struct_meta is too short. Needs at least %d bytes", *ic + nc);
		Error(message);
	      	return false;
	}
	  
	for (i = 0; i < nc; i++) {
	  cbuf[*ic] = s[i];
	  (*ic)++;
	}

	cbuf[*ic] = '\0';

	return true;
}

bool PutAttrDouble(int32 sds_id, Myhdf_attr_t *attr, double *val)
/* 
!C******************************************************************************

!Description: 'PutAttrDouble' writes an attribute from a parameter of type
 'double' to a HDF file.
 
!Input Parameters:
 sds_id         SDS id
 attr           Attribute data structure; the following fields are used:
                   name, type, nval

!Output Parameters:
 val            An array of values from the HDF attribute (converted from
                  type 'double' to the native type
 (returns)      Status:
                  'true' = okay
		      'false' = error writing the attribute information

!Team Unique Header:

 ! Design Notes:
   1. The values in the attribute are converted from the stored type to 
      'double' type.
   2. The HDF file is assumed to be open for SD (Science Data) access.
   3. If the attribute has more than 'MYHDF_MAX_NATTR_VAL' values, an error
      status is returned.
   4. Error messages are handled with the 'RETURN_ERROR' macro.
!END****************************************************************************
*/
{
    char8 val_char8[MYHDF_MAX_NATTR_VAL];
    int8 val_int8[MYHDF_MAX_NATTR_VAL];
    uint8 val_uint8[MYHDF_MAX_NATTR_VAL];
    int16 val_int16[MYHDF_MAX_NATTR_VAL];
    uint16 val_uint16[MYHDF_MAX_NATTR_VAL];
    int32 val_int32[MYHDF_MAX_NATTR_VAL];
    uint32 val_uint32[MYHDF_MAX_NATTR_VAL];
    float32 val_float32[MYHDF_MAX_NATTR_VAL];
    float64 val_float64[MYHDF_MAX_NATTR_VAL];
    int i;
    void *buf;
  
    bool status = true;
  
    if (attr->nval <= 0  ||  attr->nval > MYHDF_MAX_NATTR_VAL) {
        Error("invalid number of values");
        status = false;
    }
  
    switch (attr->type) {
      case DFNT_CHAR8:
        for (i = 0; i < attr->nval; i++) {
          if (     val[i] >= ((double)MYHDF_CHAR8H)) val_char8[i] = MYHDF_CHAR8H;
          else if (val[i] <= ((double)MYHDF_CHAR8L)) val_char8[i] = MYHDF_CHAR8L;
          else if (val[i] >= 0.0) val_char8[i] = (char8)(val[i] + 0.5);
          else                    val_char8[i] = -((char8)(-val[i] + 0.5));
        }
        buf = (void *)val_char8;
        break;
  
      case DFNT_INT8:
        for (i = 0; i < attr->nval; i++) {
          if (     val[i] >= ((double)MYHDF_INT8H)) val_int8[i] = MYHDF_INT8H;
          else if (val[i] <= ((double)MYHDF_INT8L)) val_int8[i] = MYHDF_INT8L;
          else if (val[i] >= 0.0) val_int8[i] =   (int8)( val[i] + 0.5);
          else                    val_int8[i] = -((int8)(-val[i] + 0.5));
        }
        buf = (void *)val_int8;
        break;
  
      case DFNT_UINT8:
        for (i = 0; i < attr->nval; i++) {
          if (     val[i] >= ((double)MYHDF_UINT8H)) val_uint8[i] = MYHDF_UINT8H;
          else if (val[i] <= ((double)MYHDF_UINT8L)) val_uint8[i] = MYHDF_UINT8L;
          else if (val[i] >= 0.0) val_uint8[i] =   (uint8)( val[i] + 0.5);
          else                    val_uint8[i] = -((uint8)(-val[i] + 0.5));
        }
        buf = (void *)val_uint8;
        break;
  
      case DFNT_INT16:
        for (i = 0; i < attr->nval; i++) {
          if (     val[i] >= ((double)MYHDF_INT16H)) val_int16[i] = MYHDF_INT16H;
          else if (val[i] <= ((double)MYHDF_INT16L)) val_int16[i] = MYHDF_INT16L;
          else if (val[i] >= 0.0) val_int16[i] =   (int16)( val[i] + 0.5);
          else                    val_int16[i] = -((int16)(-val[i] + 0.5));
        }
        buf = (void *)val_int16;
        break;
  
      case DFNT_UINT16:
        for (i = 0; i < attr->nval; i++) {
          if (     val[i] >= ((double)MYHDF_UINT16H)) val_uint16[i] = MYHDF_UINT16H;
          else if (val[i] <= ((double)MYHDF_UINT16L)) val_uint16[i] = MYHDF_UINT16L;
          else if (val[i] >= 0.0) val_uint16[i] =   (uint16)( val[i] + 0.5);
          else                    val_uint16[i] = -((uint16)(-val[i] + 0.5));
        }
        buf = (void *)val_uint16;
        break;
  
      case DFNT_INT32:
        for (i = 0; i < attr->nval; i++) {
          if (     val[i] >= ((double)MYHDF_INT32H)) val_int32[i] = MYHDF_INT32H;
          else if (val[i] <= ((double)MYHDF_INT32L)) val_int32[i] = MYHDF_INT32L;
          else if (val[i] >= 0.0) val_int32[i] =   (int32)( val[i] + 0.5);
          else                    val_int32[i] = -((int32)(-val[i] + 0.5));
        }
        buf = (void *)val_int32;
        break;
  
      case DFNT_UINT32:
        for (i = 0; i < attr->nval; i++) {
          if (     val[i] >= ((double)MYHDF_UINT32H)) val_uint32[i] = MYHDF_UINT32H;
          else if (val[i] <= ((double)MYHDF_UINT32L)) val_uint32[i] = MYHDF_UINT32L;
          else if (val[i] >= 0.0) val_uint32[i] =   (uint32)( val[i] + 0.5);
          else                    val_uint32[i] = -((uint32)(-val[i] + 0.5));
        }
        buf = (void *)val_uint32;
        break;
  
      case DFNT_FLOAT32:
        for (i = 0; i < attr->nval; i++) {
          if (     val[i] >= ((double)MYHDF_FLOAT32H)) val_float32[i] = MYHDF_FLOAT32H;
          else if (val[i] <= ((double)MYHDF_FLOAT32L)) val_float32[i] = MYHDF_FLOAT32L;
          else if (val[i] >= 0.0) val_float32[i] =   (float32)( val[i] + 0.5);
          else                    val_float32[i] = -((float32)(-val[i] + 0.5));
        }
        buf = (void *)val_float32;
        break;
  
      case DFNT_FLOAT64:
        if (sizeof(float64) == sizeof(double))
          buf = (void *)val;
        else {
          for (i = 0; i < attr->nval; i++)
            val_float64[i] = val[i];
          buf = (void *)val_float64;
        }
        break;
  
      default: 
      {
        Error("unimplmented type");
        status = false;
      }
    }
  
    if (SDsetattr(sds_id, attr->name, attr->type, attr->nval, buf) == FAIL) {
        Error("Error in setting attribute");
        status = false;
    }
  
    return status;
}


bool PutAttrString(int32 sds_id, Myhdf_attr_t *attr, char *string)
/* 
!C******************************************************************************

!Description: 'PutAttrString' writes an attribute from a parameter of type
 'double' to a HDF file.
 
!Input Parameters:
 sds_id         SDS id
 attr           Attribute data structure; the following fields are used:
                   name, type, nval

!Output Parameters:
 val            An array of values from the HDF attribute (converted from
                  type 'double' to the native type
 (returns)      Status:
                  'true' = okay
		      'false' = error writing the attribute information

!Team Unique Header:

 ! Design Notes:
   1. The values in the attribute are converted from the stored type to 
      'double' type.
   2. The HDF file is assumed to be open for SD (Science Data) access.
   3. If the attribute has more than 'MYHDF_MAX_NATTR_VAL' values, an error
      status is returned.
   4. Error messages are handled with the 'RETURN_ERROR' macro.
!END****************************************************************************
*/
{
    char8 val_char8[MYHDF_MAX_NATTR_VAL];
    int i;
    void *buf;
    bool status = true;
    char message[MSGLEN];
  
    if (attr->nval <= 0  ||  attr->nval > MYHDF_MAX_NATTR_VAL) {
	Error("invalid number of values");
        status = false;
    }
  
    if (attr->type != DFNT_CHAR8) {
        Error("invalid type -- not string (char8)");
        status = false;
    }
  
    if (sizeof(char8) == sizeof(char))
      buf = (void *)string;
    else {
      for (i = 0; i < attr->nval; i++) 
        val_char8[i] = (char8)string[i];
      buf = (void *)val_char8;
    }
  
    if (SDsetattr(sds_id, attr->name, attr->type, attr->nval, buf) == FAIL) {
        Error("Error in setting attribute");
        status = false;
    }
  
    return status;
}
