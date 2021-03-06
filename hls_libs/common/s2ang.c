#include "s2ang.h"
#include "s2r.h"
#include "error.h"
#include "math.h"

/* open S2 angles for read or create*/
int open_s2ang(s2ang_t *s2ang, intn access_mode) 
{
	char sdsname[500];     
	int32 sds_index;
	int32 sd_id, sds_id;
	int32 nattr, attr_index, count;
	char attr_name[200];
	int32 dimsizes[2];
	int32 rank, data_type, n_attrs;
	int32 start[2], edge[2];
	int ib;

	char message[MSGLEN];

	for (ib = 0; ib < NANG; ib++)
		s2ang->ang[ib] = NULL;
	s2ang->access_mode = access_mode;

	/* For DFACC_READ, find the image dimension from band 1.
	 * For DFACC_CREATE, image dimension is given. 
	 */
	if (s2ang->access_mode == DFACC_READ) {
		if ((s2ang->sd_id = SDstart(s2ang->fname, s2ang->access_mode)) == FAIL) {
			sprintf(message, "Cannot open for read: %s", s2ang->fname);
			Error(message);
			return(ERR_CREATE);
		}

		for (ib = 0; ib < NANG; ib++) {
			if ((sds_index = SDnametoindex(s2ang->sd_id, ANG_SDS_NAME[ib])) == FAIL) {
				sprintf(message, "Didn't find the SDS %s in %s", ANG_SDS_NAME[ib], s2ang->fname);
				Error(message);
				return(ERR_READ);
			}
			s2ang->sds_id[ib] = SDselect(s2ang->sd_id, sds_index);
	
			// Use sds_name, not ANG_SDS_NAME[ib].
			//if (SDgetinfo(s2ang->sds_id[ib], ANG_SDS_NAME[ib], &rank, dimsizes, &data_type, &nattr) == FAIL) {
			if (SDgetinfo(s2ang->sds_id[ib], sdsname, &rank, dimsizes, &data_type, &nattr) == FAIL) {
				Error("Error in SDgetinfo");
				return(ERR_READ);
			}
			s2ang->nrow = dimsizes[0];
			s2ang->ncol = dimsizes[1];

			start[0] = 0; edge[0] = dimsizes[0];
			start[1] = 0; edge[1] = dimsizes[1];
			if ((s2ang->ang[ib] = (uint16*)calloc(dimsizes[0] * dimsizes[1], sizeof(uint16))) == NULL) {
				Error("Cannot allocate memory");
				exit(1);
			}
			if (SDreaddata(s2ang->sds_id[ib], start, NULL, edge, s2ang->ang[ib]) == FAIL) {
				sprintf(message, "Error reading sds %s in %s", ANG_SDS_NAME[ib], s2ang->fname);
				Error(message);
				return(ERR_READ);
			}
			SDendaccess(s2ang->sds_id[ib]);
		}

		/* A few map projection attribute */
		/*ULX*/
		strcpy(attr_name, ULX);
		if ((attr_index = SDfindattr(s2ang->sd_id, attr_name)) == FAIL) {
                	sprintf(message, "Attribute \"%s\" not found in %s. ", attr_name, s2ang->fname);
                	Error(message);
			exit(1);
        	}
		SDattrinfo(s2ang->sd_id, attr_index, attr_name, &data_type, &count);
                if (SDreadattr(s2ang->sd_id, attr_index, &s2ang->ulx) == FAIL) {
                        sprintf(message, "Error read attribute \"%s\" in %s", attr_name, s2ang->fname);
                        Error(message);
                        return(-1);
                }

		/*ULY*/
		strcpy(attr_name, ULY);
		if ((attr_index = SDfindattr(s2ang->sd_id, attr_name)) == FAIL) {
                	sprintf(message, "Attribute \"%s\" not found in %s. ", attr_name, s2ang->fname);
                	Error(message);
			exit(1);
        	}
		SDattrinfo(s2ang->sd_id, attr_index, attr_name, &data_type, &count);
                if (SDreadattr(s2ang->sd_id, attr_index, &s2ang->uly) == FAIL) {
                        sprintf(message, "Error read attribute \"%s\" in %s", attr_name, s2ang->fname);
                        Error(message);
                        return(-1);
                }

		/* ZONEHEM */
		/*ULX*/
		strcpy(attr_name, ZONEHEM);
		if ((attr_index = SDfindattr(s2ang->sd_id, attr_name)) == FAIL) {
                	sprintf(message, "Attribute \"%s\" not found in %s. ", attr_name, s2ang->fname);
                	Error(message);
			exit(1);
        	}
		SDattrinfo(s2ang->sd_id, attr_index, attr_name, &data_type, &count);
                if (SDreadattr(s2ang->sd_id, attr_index, s2ang->zonehem) == FAIL) {
                        sprintf(message, "Error read attribute \"%s\" in %s", attr_name, s2ang->fname);
                        Error(message);
                        return(-1);
                }
                s2ang->zonehem[count] = '\0';


		SDend(s2ang->sd_id);
	}
	else if (s2ang->access_mode == DFACC_CREATE) {
		int irow, icol;
		char *dimnames[] = {"YDim_Grid", "XDim_Grid"};
		int32 comp_type;   /*Compression flag*/
		comp_info c_info;  /*Compression structure*/
		comp_type = COMP_CODE_DEFLATE;
		c_info.deflate.level = 2;     /*Level 9 would be too slow */
		rank = 2;

		if ((s2ang->sd_id = SDstart(s2ang->fname, s2ang->access_mode)) == FAIL) {
			sprintf(message, "Cannot create file: %s", s2ang->fname);
			Error(message);
			return(ERR_CREATE);
		}

		for (ib = 0; ib < NANG; ib++) {
			strcpy(sdsname, ANG_SDS_NAME[ib]);
			dimsizes[0] = s2ang->nrow;
			dimsizes[1] = s2ang->ncol;
	
			if ((s2ang->ang[ib] = (uint16*)calloc(dimsizes[0] * dimsizes[1], sizeof(uint16))) == NULL) {
				Error("Cannot allocate memory");
				exit(1);
			}
			if ((s2ang->sds_id[ib] = SDcreate(s2ang->sd_id, sdsname, DFNT_UINT16, rank, dimsizes)) == FAIL) {
				sprintf(message, "Cannot create SDS %s", sdsname);
				Error(message);
				return(ERR_CREATE);
			}    
			for (irow = 0; irow < s2ang->nrow; irow++) {
				for (icol = 0; icol < s2ang->ncol; icol++) 
					s2ang->ang[ib][irow * s2ang->ncol + icol] = ANGFILL;
			}
			PutSDSDimInfo(s2ang->sds_id[ib], dimnames[0], 0);
			PutSDSDimInfo(s2ang->sds_id[ib], dimnames[1], 1);
			SDsetcompress(s2ang->sds_id[ib], comp_type, &c_info);	
			SDsetattr(s2ang->sds_id[ib], "_FillValue", DFNT_CHAR8, strlen(ang_fillval), (VOIDP)ang_fillval);
			SDsetattr(s2ang->sds_id[ib], "scale_factor", DFNT_CHAR8, strlen(ang_scale_factor), (VOIDP)ang_scale_factor); 
			SDsetattr(s2ang->sds_id[ib], "add_offset", DFNT_CHAR8, strlen(ang_add_offset), (VOIDP)ang_add_offset); 
		}

		/* 9/23/2020 */
		SDsetattr(s2ang->sd_id, ULX, DFNT_FLOAT64, 1, (VOIDP)&s2ang->ulx);
		SDsetattr(s2ang->sd_id, ULY, DFNT_FLOAT64, 1, (VOIDP)&s2ang->uly);
		SDsetattr(s2ang->sd_id, ZONEHEM, DFNT_CHAR8, strlen(s2ang->zonehem), (VOIDP)s2ang->zonehem);
	}

	return 0;
} 




/* Read the 5km angle values and interpolate them to finer resolution */
/* Return 101 if the xml format is wrong. May 1, 2017 */
int make_smooth_s2ang(s2ang_t *s2ang, s2detfoo_t *s2detfoo, char *fname_xml)
{
	FILE *fxml;
	char line[500];
	char *pos;
	double val;
	int bandid, detid;
	char str[200]; 	/* Used in read the text file */
	int irow, icol, i, k;
	int irow5km, icol5km; 
	int rcgrid[N5KM];	/* row/col of ANGPIXSZ-meter pixels that contain the GML 5km values */
	int ret;
	int detector_has_data;
	char message[MSGLEN];

	uint16 *tmpang;
	if ((tmpang = (uint16*)calloc(s2ang->nrow * s2ang->ncol, sizeof(uint16))) == NULL) {
		Error("Cannot allocate memory");
		return(-1);
	}
	for (irow = 0; irow < s2ang->nrow; irow++) {
		for (icol = 0; icol < s2ang->ncol; icol++) 
			tmpang[irow * s2ang->ncol + icol] = ANGFILL;
	}

	/* Where to put the 5km grid point values in the ANGPIXSZ-meter (30m) grid.
	 * Note that the 5km values are not strictly evenly spaced in the finer-reso grid
	 * because the row/col numbers are integers.
  	 * nrow equals ncol, square tile.
	 */
	for (i = 0; i < N5KM; i++) {
		rcgrid[i] = floor(i * 5000.0/ANGPIXSZ);
		if (rcgrid[i] > s2ang->nrow-1)  /* The grid cells at the right edge and bottom edge are smaller */
			rcgrid[i] = s2ang->nrow-1; 
	}

	/* The granule's xml */
	if ((fxml = fopen(fname_xml, "r")) == NULL) {
		sprintf(message, "Cannot read %s", fname_xml);
		Error(message);
		return(-1);
	}
	while (fgets(line, sizeof(line), fxml)) {

		/* Sun angles, same for all bands */
		if (strstr(line, "<Sun_Angles_Grid>")) {
			fgets(line, sizeof(line), fxml);
			if (strstr(line, "<Zenith>") == NULL) {
				sprintf(message, "Format is not as expected: %s", fname_xml);
				Error(message);
				exit(1);
			}

			/*** Solar zenith ***/
			fgets(line, sizeof(line), fxml); /* <COL_STEP unit="m">5000</COL_STEP> */
			fgets(line, sizeof(line), fxml); /* <ROW_STEP unit="m">5000</ROW_STEP> */
			fgets(line, sizeof(line), fxml); /* <Values_List> */

			/* Read the 5km values to save at the finer-resolution location (i.e. ANGPIXSZ meter) */
			for (irow5km = 0; irow5km < N5KM; irow5km++) {
				irow = rcgrid[irow5km]; 	
				for (icol5km = 0; icol5km < N5KM; icol5km++) {
					icol = rcgrid[icol5km]; 
					k = irow * s2ang->ncol + icol;
					fscanf(fxml, "%s", str);
					if (icol5km == 0)
						val = atof(str+strlen("<VALUES>"));
					else 	
						val = atof(str);

					s2ang->ang[0][k] = val * 100;
				}
			}


			/* interp */
			ret = interp_s2ang_bilinear(s2ang->ang[0], s2ang->nrow, s2ang->ncol);
			if (ret != 0) {
				sprintf(message, "Error in interp_s2ang_bilinear for %s", fname_xml);
				Error(message);
				return(-1);
			}

			/*** Solar azimuth***/
			fgets(line, sizeof(line), fxml);	/* There is a "\n" trailing "</VALUES>" unread by fscanf*/
			fgets(line, sizeof(line), fxml);	/* </Values_List> */
			fgets(line, sizeof(line), fxml);	/* </Zenith> */
			fgets(line, sizeof(line), fxml);	/* <Azimuth> */
			if (strstr(line, "<Azimuth>") == NULL) {	/* Good that I checked */
				sprintf(message, "Format is not as expected: %s", fname_xml);
				Error(message);
				exit(1);
			}
			fgets(line, sizeof(line), fxml); /* <COL_STEP unit="m">5000</COL_STEP> */
			fgets(line, sizeof(line), fxml); /* <ROW_STEP unit="m">5000</ROW_STEP> */
			fgets(line, sizeof(line), fxml); /* <Values_List> */
			//fprintf(stderr, "%s\n", line);
			
			for (irow5km = 0; irow5km < N5KM; irow5km++) {
				irow = rcgrid[irow5km]; 	
				for (icol5km = 0; icol5km < N5KM; icol5km++) {
					icol = rcgrid[icol5km]; 
					k = irow * s2ang->ncol + icol;
					fscanf(fxml, "%s", str);
					if (icol5km == 0)
						val = atof(str+strlen("<VALUES>"));
					else 	
						val = atof(str);

					s2ang->ang[1][k] = val * 100;
				}
			}

			/* interp */
			ret = interp_s2ang_bilinear(s2ang->ang[1], s2ang->nrow, s2ang->ncol);
			if (ret != 0) {
				sprintf(message, "Error in interp_s2ang_bilinear for %s", fname_xml);
				Error(message);
				return(-1);
			}
		}


		/* View angles for each band and each detector*/
		/* For each band, the view zenith and view azimuth angles are given one detector after another. */
		/* Sep 7, 2020: Now only derive for B06, whose bandId is 5. Use the angle data for all bands */
		if (strstr(line, "<Viewing_Incidence_Angles_Grids bandId=")) {
			pos = strstr(line, "bandId=\"");
			bandid = atoi(pos+strlen("bandId=\""));		/* bandId is 0-based*/
			pos = strstr(line, "detectorId=\"");	/* 1-based; not used in indexing */
			detid = atoi(pos+ strlen("detectorId=\""));
			fgets(line, sizeof(line), fxml);	/* <Zenith> */
			if (strstr(line, "<Zenith>") == NULL) {
				sprintf(message, "Format is not as expected: %s", fname_xml);
				Error(message);
				exit(1);
			}

			/*** View Zenith ***/
			fgets(line, sizeof(line), fxml); /* <COL_STEP unit="m">5000</COL_STEP> */
			fgets(line, sizeof(line), fxml); /* <ROW_STEP unit="m">5000</ROW_STEP> */
			fgets(line, sizeof(line), fxml); /* <Values_List> */

			/* May 1, 2017: When the information is not available it can be:
			 <COL_STEP unit="m">0</COL_STEP>
          		 <ROW_STEP unit="m">0</ROW_STEP>
          		 <Values_List>
            		   <VALUES>0</VALUES>
            		   <VALUES>0</VALUES>
			*/

			/* Init */
			/* If a detector's footprint is not on the tile, some processing baselines still gave
			 * the angles for the detector (of course all NaN).
			 */
			detector_has_data = 0;
			for (irow = 0; irow < s2ang->nrow; irow++) {
				for (icol = 0; icol < s2ang->ncol; icol++) 
					tmpang[irow * s2ang->ncol + icol] = ANGFILL;
			}
			for (irow5km = 0; irow5km < N5KM; irow5km++) {
				irow = rcgrid[irow5km];  /* the fine-resolution rows in which 5km values are available. */ 
				for (icol5km = 0; icol5km < N5KM; icol5km++) {
					icol = rcgrid[icol5km]; /* The fine-reso cols where 5km values avail. */ 
					k = irow * s2ang->ncol + icol;

					/* No matter what, scan it first (consume the data in the stream). */
					fscanf(fxml, "%s", str);

					if (icol5km == 0) {
						if (strncmp(str, "<VALUES>", strlen("<VALUES>")) != 0) {
							sprintf(message,"irow5km = %d, xml format wrong, or is not read correctly: %s\n", irow5km, fname_xml);
							Error(message);
							exit(101);
						}

						if (strstr(str, "NaN")) 
							val = ANGFILL;
						else 
							val = atof(str+strlen("<VALUES>"));
					}
					else {	
						if (strstr(str, "NaN"))
							val = ANGFILL;
						else
							val = atof(str);
					}


					if (val != ANGFILL) {
						tmpang[k] = val * 100;
						detector_has_data = 1;
					}
				}
			}
			fgets(line, sizeof(line), fxml);	/* There is a "\n" trailing "</VALUES>" */
			fgets(line, sizeof(line), fxml);	/* </Values_List> */
			fgets(line, sizeof(line), fxml);	/* </Zenith> */
			if (bandid == 5 && detector_has_data) {		/* Use the 2nd red-edge band */
				/* interp */
				ret = interp_s2ang_bilinear(tmpang, s2ang->nrow, s2ang->ncol);
				if (ret != 0) {
					sprintf(message, "Error in interp_s2ang_bilinear for bandId %d, detectorId %d: %s", bandid, detid, fname_xml);
					Error(message);
					return(-1);
				}
				/* Cookiecut with the footprint */
				for (irow = 0; irow < s2ang->nrow; irow++) {
					for (icol = 0; icol < s2ang->ncol; icol++) {
						k = irow * s2ang->ncol + icol;
						if (detid == s2detfoo->detid[k])
							s2ang->ang[2][k] = tmpang[k];

					}
				}
			}


			/*** View Azimuth ***/
			fgets(line, sizeof(line), fxml);	/* <Azimuth> */
			if (strstr(line, "<Azimuth>") == NULL) {	/* Good that I checked */
				sprintf(message, "Format is not as expected: %s", fname_xml);
				Error(message);
				exit(1);
			}
			fgets(line, sizeof(line), fxml); /* <COL_STEP unit="m">5000</COL_STEP> */
			fgets(line, sizeof(line), fxml); /* <ROW_STEP unit="m">5000</ROW_STEP> */
			fgets(line, sizeof(line), fxml); /* <Values_List> */
			
			/* Init */
			detector_has_data = 0;
			for (irow = 0; irow < s2ang->nrow; irow++) {
				for (icol = 0; icol < s2ang->ncol; icol++) 
					tmpang[irow * s2ang->ncol + icol] = ANGFILL;
			}
			for (irow5km = 0; irow5km < N5KM; irow5km++) {
				irow = rcgrid[irow5km]; 	
				for (icol5km = 0; icol5km < N5KM; icol5km++) {
					icol = rcgrid[icol5km]; 
					k = irow * s2ang->ncol + icol;

					/* No matter what, scan it first. */
					fscanf(fxml, "%s", str);

					if (icol5km == 0) {
						if (strncmp(str, "<VALUES>", strlen("<VALUES>")) != 0) {
							sprintf(message, "Format is not as expected: %s", fname_xml);
							Error(message);
							exit(1);
						}

						if (strstr(str, "NaN")) 
							val = ANGFILL;
						else 
							val = atof(str+strlen("<VALUES>"));
					}
					else {	
						if (strstr(str, "NaN"))
							val = ANGFILL;
						else
							val = atof(str);
					}

					if (val != ANGFILL) {
						tmpang[k] = val * 100;
						detector_has_data = 1;
					}
				}
			}
			if (bandid == 5 && detector_has_data) {
				/* interp */
				ret = interp_s2ang_bilinear(tmpang, s2ang->nrow, s2ang->ncol);
				if (ret != 0) {
					sprintf(message, "Error in interp_s2ang_bilinear for bandId %d, detectorId %d: %s", bandid, detid, fname_xml);
					Error(message);
					return(-1);
				}
				/* Cookiecut with the footprint */
				for (irow = 0; irow < s2ang->nrow; irow++) {
					for (icol = 0; icol < s2ang->ncol; icol++) {
						k = irow * s2ang->ncol + icol;
						if (detid == s2detfoo->detid[k])
							s2ang->ang[3][k] = tmpang[k];
					}
				}
			}
		} /* Angles for all bands are examined; recorded for B06 */
	}

	free(tmpang);

	return 0;
}

/* Bilinear interpolation from the 5km angle grid, which can be solar zenith or azimuth, 
 * or view zenith or azimuth for a single detector. When it is the angle for a single 
 * detector, the 5km values are interpolated (extrapolated) to the whole image for 
 * easiler implementation. This will not create any problem since only the values within 
 * a detector's footprint will be used later.
 */
int interp_s2ang_bilinear(uint16 *ang, int nrow, int ncol)
{
	int rcgrid[N5KM];	/* row/col of ANGPIXSZ meters that contain the 5km values */
	char valid5km[N5KM]; 	/* A mask of valid 5km data points for a row*/
	int lftcol, rgtcol;	
	int toprow, botrow;

	int irow, icol;
	int irow5km, icol5km;	
	int i,k, k5km; 
	int row5km_start, row5km_end,
	    col5km_start, col5km_end;
	int lftcol5km, rgtcol5km, toprow5km, botrow5km;
	int tmprow, tmpcol;
	uint16 anglft, angrgt, angtop, angbot;
	double angint;		/* interpolated angle */
	char message[MSGLEN];

	/* Compute the row and col locations of fine-resolution ANGPIXSZ pixels where the original 5km 
         * angle is available.  Not a regularly spaced grid because when the 5km values are saved at 
         * fine resolution, the spacing between values can be +/- one fine-reso pixel due to rounding. 
         * Additionally, the spacing between the the last two fine-reso rows or cols which hold the 
         * 5km values is smaller (because 23 point * 5km > 109800 meters).  
	 *
	 * This code snippet also appears in the calling function.  
	 * This array could have been passed in.  Too late to change, Nov 26, 2019. 
	 */
	for (i = 0; i < N5KM; i++) {
		rcgrid[i] = floor(i * 5000.0/ANGPIXSZ);
		if (rcgrid[i] > nrow-1)	/* or ncol-1. Square tile */
			rcgrid[i] = nrow-1;
	}

	/* First pass: linear interp on the rows where 5km data are available.
	 * Note that only N5KM rows have the 5km data.
	 */
	for (irow5km = 0; irow5km < N5KM; irow5km++) {

		/* Set the mask of valid 5km data for this row; later interpolate/extrapolation for a row
                 * only use data from these points, but not from interpolated or extrapolated ones.
 		 */
		irow = rcgrid[irow5km];		/* This fine-resolution row may have the 5km data (Fill or not) */
		for (icol5km = 0; icol5km < N5KM; icol5km++) {
			icol = rcgrid[icol5km];	/* This fine-resol col may have the 5km data (Fill or not) */
			if (ang[irow * ncol + icol] == ANGFILL)
				valid5km[icol5km] = 0;
			else
				valid5km[icol5km] = 1;
		}


		/* The first and last column in the 5km grid that hold valid data for this row.
 		 * There can be holes between valid data points; for example: In
 		 * S2A_MSIL1C_20190727T213051_N0208_R086_T17XMK_20190728T010507.SAFE
 		 * Band 01 view zenith:
 		 * <Viewing_Incidence_Angles_Grids bandId="0" detectorId="8">
        	 * <Zenith>
          	 *<COL_STEP unit="m">5000</COL_STEP>
          	 *<ROW_STEP unit="m">5000</ROW_STEP>
          	 *<Values_List>
            	 *<VALUES>3.16347 3.19559 3.2289 3.26041 NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN NaN 3.80201 3.83812 3.87265 3.90644</VALUES>
            	 * <VALUES>3.49812 3.53211 3.56407 3.59715 3.63095 3.66483 3.70045 3.73548 3.77001 3.80429 3.84005 3.8748 3.90944 3.94418 3.98009 4.0149 4.05163 4.08518 4.11796 4.15563 4.19345 4.22708 4.26125</VALUES>
 		 *
 		 * Nov 26, 2019.
 		 *
 		 * In v1.4 the mask valid5km was not used; using col5km_start and col5km_end alone made mistakes for the above case.
 		 */
		col5km_start = -1;
		col5km_end = -1;
		for (icol5km = 0; icol5km < N5KM; icol5km++) {	 
			if (valid5km[icol5km] == 1 && col5km_start == -1) 
				col5km_start = icol5km;

			if (valid5km[icol5km] == 1)
				col5km_end = icol5km;
		}

		/* No valid 5km data on this row. Done with row */ 
		if (col5km_start == -1) {
			// Not an error.  This is possible when a detector's footprint does not extend
			// from the very top to the very bottom of the tile, but on the upper-left or 
			// lower-right corner of the tile. Then for some rows there are no valid angle
			// data at all.  
			continue;
		}

		for (icol = 0; icol < ncol; icol++) {
			/* For each fine-reso pixel on a row, find its two nearest
 			 * neighbors that hold valid 5km data. For the ideal case, the two neighbors
 			 * enclose the fine-reso pixel spatially, but sometimes they don't and 
 			 * extrapolation is applied.
 			 * Remember: there can be 5km-holes between valid 5km-points.
			 */
			lftcol5km = -1;
			rgtcol5km = -1;

			/* The left 5km point */
			for (icol5km = col5km_end; icol5km >= col5km_start; icol5km--) {
				if (icol >= rcgrid[icol5km] && valid5km[icol5km] == 1) {
					lftcol5km = icol5km;
					break;
				}
			}
			/* Won't be enclosing: fine-resolution point is outside on the left  */
			if (lftcol5km == -1)
				lftcol5km = col5km_start;

			/* The right 5km point */
			for (icol5km = col5km_start; icol5km <= col5km_end; icol5km++) {
				if (icol <= rcgrid[icol5km] && valid5km[icol5km] == 1) {
					rgtcol5km = icol5km;
					break;
				}
			}
			/* Won't be enclosing: fine-resolution point is outside on the right  */
			if (rgtcol5km == -1)
				rgtcol5km = col5km_end;


			/* If the two found 5km points enclose the fine-reso pixel, it is interpolation. 
 			 *
 			 * If they don't, it becomes extrapolation, which can be wild in that extrapolated values 
 			 * can be nagative or extremely high, or coincidentally be the fill value. But this 
 			 * does NOT pose a problem because later the detector's footprint is used to cookie-cut 
 			 * the angle values and the points with extrapolated values should be outside the
 			 * detector's footprint anyway.
			 */
			lftcol = rcgrid[lftcol5km];	
			rgtcol = rcgrid[rgtcol5km];

			anglft = ang[irow * ncol + lftcol];
			angrgt = ang[irow * ncol + rgtcol];

			if (lftcol5km == rgtcol5km) 
				ang[irow * ncol + icol] = anglft;   /* or angrgt; Only one 5km value on this row */
			else { 
				/* Interpolation or extrapolation */
				angint = anglft + (angrgt - anglft) * 1.0 / (rgtcol - lftcol) * (icol - lftcol);
				ang[irow * ncol + icol] = angint;
			}
		}
	}

	/* Second pass: linear interpolation/extrapolation for the pixels in all columns is possible since all 
 	 * fine-reso columns should have at least one non-fill data value after first pass. 
	 */
	for (icol = 0; icol < ncol; icol++) {

		/* The first row and last row in the 5km grid that holds data for this column. The data can
 		 * be original 5km data or from interpolation/extrapolation; that is, we no longer insist that
 		 * interpolation/extrapolation must be based on original 5km data, in the way we do in the first pass.
 		 */
		row5km_start = -1;
		row5km_end = -1;
		for (irow5km = 0; irow5km < N5KM; irow5km++) {	 
			k = rcgrid[irow5km] * ncol + icol;
			if (ang[k] != ANGFILL && row5km_start == -1) 
				row5km_start = irow5km;

			if (ang[k] != ANGFILL)
				row5km_end = irow5km;

			/* Think about: how likely is does interpolated/extrapolated value from the first 
 			 * pass happen to be ANGFILL?   Ignore for now. Nov 26, 2019*/
		}
		

		for (irow = 0; irow < nrow; irow++) {	

			/* For each fine-reso pixel on a column, find its two nearest
	 		 * neighbors on the 5km grid that have angle data. The data can be original 5km data
	 		 * or interpolated/extrapolated ones.
			 */
			toprow5km = -1;
			botrow5km = -1;

			/* The top row in the 5km grid */
			for (irow5km = row5km_end; irow5km >= row5km_start; irow5km--) {
				k = rcgrid[irow5km] * ncol + icol;
				if (irow >= rcgrid[irow5km] && ang[k] != ANGFILL) {
					toprow5km = irow5km;
					break;
				}
			}
			/* Won't be enclosing: fine-resolution point is outside on the top */
			if (toprow5km == -1)
				toprow5km = row5km_start;
	
			/* The bottom row in the 5km grid*/
			for (irow5km = row5km_start; irow5km <= row5km_end; irow5km++) {
				k = rcgrid[irow5km] * ncol + icol;
				if (irow <= rcgrid[irow5km] && ang[k] != ANGFILL) {
					botrow5km = irow5km;
					break;
				}
			}
			/* Won't be enclosing: fine-resolution point is outside at the bottom*/
			if (botrow5km == -1)
				botrow5km = row5km_end;


			toprow = rcgrid[toprow5km];
			botrow = rcgrid[botrow5km];
			angtop = ang[toprow * ncol + icol];
			angbot = ang[botrow * ncol + icol];

			if (toprow5km == botrow5km) { 	/* Only one value in this column */
				k = rcgrid[toprow5km] * ncol + icol;
				ang[irow * ncol + icol] = ang[k];
			} else {
				/* Interp or Extrap */
				angint = angtop + (angbot - angtop) * 1.0 / (botrow - toprow) * (irow - toprow);
				ang[irow * ncol + icol] = angint;
			}
		}
	}

	return 0;
}


/* close */
int close_s2ang(s2ang_t *s2ang)
{
	char message[MSGLEN];
	int ib;
	int nsds = 4;

	if (s2ang->access_mode == DFACC_CREATE && s2ang->sd_id != FAIL) {
		char sdsname[500];     
		int32 start[2], edge[2];

		start[0] = 0; edge[0] = s2ang->nrow;
		start[1] = 0; edge[1] = s2ang->ncol;


		for (ib = 0; ib < NANG; ib++) {
			if (SDwritedata(s2ang->sds_id[ib], start, NULL, edge, s2ang->ang[ib]) == FAIL) {
				Error("Error in SDwritedata");
				return(ERR_CREATE);
			}
			SDendaccess(s2ang->sds_id[ib]);
		}

		SDend(s2ang->sd_id);
		s2ang->sd_id = FAIL;
	}


	/* free up memory */
	for (ib = 0; ib < NANG; ib++) {
		if (s2ang->ang[ib] != NULL) {
			free(s2ang->ang[ib]);
			s2ang->ang[ib] = NULL;
		}
	}

	return 0;
}

