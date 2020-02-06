#include "s2ang.h"
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

	s2ang->sz = NULL;
	s2ang->sa = NULL;
	for (ib = 0; ib < S2NBAND; ib++) {
		s2ang->vz[ib] = NULL;
		s2ang->va[ib] = NULL;
	}
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

		/* SZ */
		strcpy(sdsname, SZ);
		if ((sds_index = SDnametoindex(s2ang->sd_id, sdsname)) == FAIL) {
			sprintf(message, "Didn't find the SDS %s in %s", sdsname, s2ang->fname);
			Error(message);
			return(ERR_READ);
		}
		s2ang->sds_id_sz = SDselect(s2ang->sd_id, sds_index);

		if (SDgetinfo(s2ang->sds_id_sz, sdsname, &rank, dimsizes, &data_type, &nattr) == FAIL) {
			Error("Error in SDgetinfo");
			return(ERR_READ);
		}
		s2ang->nrow = dimsizes[0];
		s2ang->ncol = dimsizes[1];
		start[0] = 0; edge[0] = dimsizes[0];
		start[1] = 0; edge[1] = dimsizes[1];
		if ((s2ang->sz = (uint16*)calloc(dimsizes[0] * dimsizes[1], sizeof(uint16))) == NULL) {
			Error("Cannot allocate memory");
			exit(1);
		}
		if (SDreaddata(s2ang->sds_id_sz, start, NULL, edge, s2ang->sz) == FAIL) {
			sprintf(message, "Error reading sds %s in %s", sdsname, s2ang->fname);
			Error(message);
			return(ERR_READ);
		}
		SDendaccess(s2ang->sds_id_sz);

		/* SA */
		strcpy(sdsname, SA);
		if ((sds_index = SDnametoindex(s2ang->sd_id, sdsname)) == FAIL) {
			sprintf(message, "Didn't find the SDS %s in %s", sdsname, s2ang->fname);
			Error(message);
			return(ERR_READ);
		}
		s2ang->sds_id_sa = SDselect(s2ang->sd_id, sds_index);

		if ((s2ang->sa = (uint16*)calloc(dimsizes[0] * dimsizes[1], sizeof(uint16))) == NULL) {
			Error("Cannot allocate memory");
			exit(1);
		}
		if (SDreaddata(s2ang->sds_id_sa, start, NULL, edge, s2ang->sa) == FAIL) {
			sprintf(message, "Error reading sds %s in %s", sdsname, s2ang->fname);
			Error(message);
			return(ERR_READ);
		}
		SDendaccess(s2ang->sds_id_sa);

		/* View zenith and azimuth for all bands */
		for (ib = 0; ib < S2NBAND; ib++) {
			/* zenith */
			sprintf(sdsname, "%s_%s", S2_SDS_NAME[ib], VZ);
			if ((sds_index = SDnametoindex(s2ang->sd_id, sdsname)) == FAIL) {
				sprintf(message, "Didn't find the SDS %s in %s", sdsname, s2ang->fname);
				Error(message);
				return(ERR_READ);
			}
			s2ang->sds_id_vz[ib] = SDselect(s2ang->sd_id, sds_index);

			if ((s2ang->vz[ib] = (uint16*)calloc(dimsizes[0] * dimsizes[1], sizeof(uint16))) == NULL) {
				Error("Cannot allocate memory");
				exit(1);
			}
			if (SDreaddata(s2ang->sds_id_vz[ib], start, NULL, edge, s2ang->vz[ib]) == FAIL) {
				sprintf(message, "Error reading sds %s in %s", sdsname, s2ang->fname);
				Error(message);
				return(ERR_READ);
			}
			SDendaccess(s2ang->sds_id_vz[ib]);

			/* azimuth*/
			sprintf(sdsname, "%s_%s", S2_SDS_NAME[ib], VA);
			if ((sds_index = SDnametoindex(s2ang->sd_id, sdsname)) == FAIL) {
				sprintf(message, "Didn't find the SDS %s in %s", sdsname, s2ang->fname);
				Error(message);
				return(ERR_READ);
			}
			s2ang->sds_id_va[ib] = SDselect(s2ang->sd_id, sds_index);

			if ((s2ang->va[ib] = (uint16*)calloc(dimsizes[0] * dimsizes[1], sizeof(uint16))) == NULL) {
				Error("Cannot allocate memory");
				exit(1);
			}
			if (SDreaddata(s2ang->sds_id_va[ib], start, NULL, edge, s2ang->va[ib]) == FAIL) {
				sprintf(message, "Error reading sds %s in %s", sdsname, s2ang->fname);
				Error(message);
				return(ERR_READ);
			}
			SDendaccess(s2ang->sds_id_va[ib]);
		}

		/* Read ANGLEAVAIL attribute */
		strcpy(attr_name, ANGLEAVAIL);
		if ((attr_index = SDfindattr(s2ang->sd_id, attr_name)) == FAIL) {
			sprintf(message, "Attribute \"%s\" not found in %s", attr_name, s2ang->fname);
			Error(message);
			return (-1);
		}
		if (SDattrinfo(s2ang->sd_id, attr_index, attr_name, &data_type, &count) == FAIL) {
			sprintf(message, "Error in SDattrinfo for %s", s2ang->fname);
			Error(message);
			return(-1);
		}
		if (SDreadattr(s2ang->sd_id, attr_index, s2ang->angleavail) == FAIL) {
			sprintf(message, "Error read attribute \"%s\" in %s", attr_name, s2ang->fname);
			Error(message);
			return(-1);
		}

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

		/* SZ */
		strcpy(sdsname, SZ);
		dimsizes[0] = s2ang->nrow;
		dimsizes[1] = s2ang->ncol;

		if ((s2ang->sz = (uint16*)calloc(dimsizes[0] * dimsizes[1], sizeof(uint16))) == NULL) {
			Error("Cannot allocate memory");
			exit(1);
		}
		if ((s2ang->sds_id_sz = SDcreate(s2ang->sd_id, sdsname, DFNT_UINT16, rank, dimsizes)) == FAIL) {
			sprintf(message, "Cannot create SDS %s", sdsname);
			Error(message);
			return(ERR_CREATE);
		}
		for (irow = 0; irow < s2ang->nrow; irow++) {
			for (icol = 0; icol < s2ang->ncol; icol++)
				s2ang->sz[irow * s2ang->ncol + icol] = ANGFILL;
		}
		PutSDSDimInfo(s2ang->sds_id_sz, dimnames[0], 0);
		PutSDSDimInfo(s2ang->sds_id_sz, dimnames[1], 1);
		SDsetcompress(s2ang->sds_id_sz, comp_type, &c_info);

		/* SA */
		strcpy(sdsname, SA);
		dimsizes[0] = s2ang->nrow;
		dimsizes[1] = s2ang->ncol;

		if ((s2ang->sa = (uint16*)calloc(dimsizes[0] * dimsizes[1], sizeof(uint16))) == NULL) {
			Error("Cannot allocate memory");
			exit(1);
		}
		if ((s2ang->sds_id_sa = SDcreate(s2ang->sd_id, sdsname, DFNT_UINT16, rank, dimsizes)) == FAIL) {
			sprintf(message, "Cannot create SDS %s", sdsname);
			Error(message);
			return(ERR_CREATE);
		}
		for (irow = 0; irow < s2ang->nrow; irow++) {
			for (icol = 0; icol < s2ang->ncol; icol++)
				s2ang->sa[irow * s2ang->ncol + icol] = ANGFILL;
		}
		PutSDSDimInfo(s2ang->sds_id_sa, dimnames[0], 0);
		PutSDSDimInfo(s2ang->sds_id_sa, dimnames[1], 1);
		SDsetcompress(s2ang->sds_id_sa, comp_type, &c_info);


		/* View angles for each band */
		for (ib = 0; ib < S2NBAND; ib++) {
			/* VZ */
			sprintf(sdsname, "%s_%s", S2_SDS_NAME[ib], VZ);
			dimsizes[0] = s2ang->nrow;
			dimsizes[1] = s2ang->ncol;

			if ((s2ang->vz[ib] = (uint16*)calloc(dimsizes[0] * dimsizes[1], sizeof(uint16))) == NULL) {
				Error("Cannot allocate memory");
				exit(1);
			}
			if ((s2ang->sds_id_vz[ib] = SDcreate(s2ang->sd_id, sdsname, DFNT_UINT16, rank, dimsizes)) == FAIL) {
				sprintf(message, "Cannot create SDS %s", sdsname);
				Error(message);
				return(ERR_CREATE);
			}    
			PutSDSDimInfo(s2ang->sds_id_vz[ib], dimnames[0], 0);
			PutSDSDimInfo(s2ang->sds_id_vz[ib], dimnames[1], 1);
			SDsetcompress(s2ang->sds_id_vz[ib], comp_type, &c_info);	
			for (irow = 0; irow < s2ang->nrow; irow++) {
				for (icol = 0; icol < s2ang->ncol; icol++) 
					s2ang->vz[ib][irow * s2ang->ncol + icol] = ANGFILL;
			}
	
			/* VA */
			sprintf(sdsname, "%s_%s", S2_SDS_NAME[ib], VA);
			dimsizes[0] = s2ang->nrow;
			dimsizes[1] = s2ang->ncol;
	
			if ((s2ang->va[ib] = (uint16*)calloc(dimsizes[0] * dimsizes[1], sizeof(uint16))) == NULL) {
				Error("Cannot allocate memory");
				exit(1);
			}
			if ((s2ang->sds_id_va[ib] = SDcreate(s2ang->sd_id, sdsname, DFNT_UINT16, rank, dimsizes)) == FAIL) {
				sprintf(message, "Cannot create SDS %s", sdsname);
				Error(message);
				return(ERR_CREATE);
			}    
			PutSDSDimInfo(s2ang->sds_id_va[ib], dimnames[0], 0);
			PutSDSDimInfo(s2ang->sds_id_va[ib], dimnames[1], 1);
			SDsetcompress(s2ang->sds_id_va[ib], comp_type, &c_info);	
			for (irow = 0; irow < s2ang->nrow; irow++) {
				for (icol = 0; icol < s2ang->ncol; icol++) 
					s2ang->va[ib][irow * s2ang->ncol + icol] = ANGFILL;
			}
		}
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

	for (i = 0; i < S2NBAND; i++)
		s2ang->angleavail[i] = 0;

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

			/* Solar zenith */
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

					s2ang->sz[k] = val * 100;
				}
			}

			/* interp */
			ret = interp_s2ang_bilinear(s2ang->sz, s2ang->nrow, s2ang->ncol);
			if (ret != 0) {
				sprintf(message, "Error in interp_s2ang_bilinear for %s", fname_xml);
				Error(message);
				return(-1);
			}

			/* Solar azimuth*/
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

					s2ang->sa[k] = val * 100;
				}
			}

			/* interp */
			ret = interp_s2ang_bilinear(s2ang->sa, s2ang->nrow, s2ang->ncol);
			if (ret != 0) {
				sprintf(message, "Error in interp_s2ang_bilinear for %s", fname_xml);
				Error(message);
				return(-1);
			}
		}


		/* View angles for each band and each detector*/
		/* For each band, the view zenith and view azimuth angles are given one detector after another. */
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

			s2ang->angleavail[bandid] = 1;

			/*************** Zenith ********************/
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
			if (detector_has_data) {
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
						if (detid == s2detfoo->detid[bandid][k])
							s2ang->vz[bandid][k] = tmpang[k];

					}
				}
			}


			/*************** Azimuth ********************/
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
			if (detector_has_data) {
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
						if (detid == s2detfoo->detid[bandid][k])
							s2ang->va[bandid][k] = tmpang[k];
					}
				}
			}
		} /* Angles for each band */
	}

	/* Sep 6, 2016: Test if all bands are accounted for */
	for (i = 0; i < S2NBAND; i++) {
		char bandstr[10];
		if ( ! s2ang->angleavail[i]) {
			if (i <= 7)
				sprintf(bandstr, "%d", i+1);
			else if (i == 8)
				sprintf(bandstr, "8A");
			else
				sprintf(bandstr, "%d", i);
			sprintf(message, "Band %s not available in %s", bandstr, fname_xml);
			Error(message);
		}
	}

	free(tmpang);
	return 0;
}

/* Bilinear interpolation from the 5km angle grid, which can be solar zenith or azimuth, 
 * or view zenig or azimuth for a single detector. When it is the angle for a single 
 * detector, the 5km values are interpolated (extrapolated) to the whole image for 
 * easiler implementation. This will not create any problem since only the values within 
 * a detector's footprint will be used later.
 */
int interp_s2ang_bilinear(uint16 *ang, int nrow, int ncol)
{
	/* The row/col number that have the 5km angle values */
	int rcgrid[N5KM];	/* row/col of ANGPIXSZ meters that contain the 5km values */
	int lftcol, rgtcol;	
	int toprow, botrow;

	int irow, icol;
	int irow5km, icol5km;	
	int i,k, k5km; 
	int row5km_start, row5km_end,
	    col5km_start, col5km_end;
	int lftcol5km, rgtcol5km, toprow5km, botrow5km;
	int tmprow, tmpcol;
	int search_start;	/* an index */
	uint16 anglft, angrgt, angtop, angbot;
	double angint;		/* interpolated angle */
	char message[MSGLEN];

	/* Compute the row/col locations of ANGPIXSZ pixels where the original 5km angle is available. 
	 * Not a regularly spaced grid because when the 5km values are saved at fine resolution, the 
	 * spacing between values can be +/- one fine-reso pixel. Additionally, the spacing between the
	 * the last two fine-reso rows or cols which hold the 5km values is smaller.
	 * Originally an output pixel size 10m was chose and then every 500 pixels there was an 5km 
	 * original input. But now ANGPIXSZ (30m) is used to reduce the data volume, and the grid 
	 * becomes slightly irregular. So use an array "rcgrid" to save where 5km values are saved.
	 */
	/* This code snippet also appears in the calling function */
	for (i = 0; i < N5KM; i++) {
		rcgrid[i] = floor(i * 5000.0/ANGPIXSZ);
		if (rcgrid[i] > nrow-1)	/* or ncol-1, square tile */
			rcgrid[i] = nrow-1;
	}
	
	/* First pass: linear interp for the pixels on the rows which hold the 5km data. 
	 * Note that only N5KM rows have the 5km data.
	 */
	for (irow5km = 0; irow5km < N5KM; irow5km++) {
		irow = rcgrid[irow5km];	/* This row may have the 5km data (Fill or not) */
		col5km_start = -1;
		col5km_end = -1;
		for (icol5km = 0; icol5km < N5KM; icol5km++) {	 /* Find the range of columns which have the 5km data */
			icol = rcgrid[icol5km];
			k = irow * ncol + icol; 
			if (ang[k] != ANGFILL && col5km_start == -1) 
				col5km_start = icol5km;
			if (ang[k] != ANGFILL) 
				col5km_end = icol5km;
		}

		if (col5km_start == -1) {
			// Not an error. This is possible when a detector's footprint does not extend
			// from the very top to the very bottom, but on the upper-left or 
			// lower-right corner of the tile.  
			continue;
		}
		else if (col5km_start == col5km_end ) {
			/* If there is only one 5km value on this line, just use this for every pixel. */
			k = irow * ncol + rcgrid[col5km_start];
			for (icol = 0; icol < ncol; icol++) {
				ang[irow * ncol + icol] = ang[k];
			}
		}
		else {
			search_start = 0;
			for (icol = 0; icol < ncol; icol++) {
				/* For each fine-reso pixel on a row holding the 5km values, find its two 
				 * nearest neighbors where the 5km values are potentially available. 
				 * The neighbors can have fill value though.
				 */
				for (icol5km = search_start; icol5km < N5KM-1; icol5km++) {
					if (icol >= rcgrid[icol5km] && icol <= rcgrid[icol5km+1]) {
						lftcol5km = icol5km;
						rgtcol5km = icol5km+1;
					}
				}
	
				/* If both neighbors are non-fill, it is interpolation. If either neighbor has fill 
				 * value, move along the row to find two nearest non-fill neighbors 
				 * and then interp becomes extrap.
				 * Bug fix: test whether a neighbor is within col5km_start and col5km_end, rather than
				 *          whether a neighbor has fill value because as the extrapolation goes on,
				 * 	    the originally fillvalue becomes nonfill!   Sep 4, 2016.
				 *
				 * Note: As the target location moves away from [col5km_start, col5km_start+1], the
			 	 * extrapolation can become wild -- extrapolated values can be nagative or extremely high, or
				 * coincidentally be the fill value. But this does pose a problem because later
				 * the detector's footprint is used to cookie-cut the angle values. As a result, the 
				 * retained values are interpolated, or extraploated around the footprint; the values for far away
				 * locations are not used at all (they are in other dectors' footprint).
				 */
				if (lftcol5km < col5km_start) {
					lftcol5km = col5km_start;	
					rgtcol5km = col5km_start+1;
				} 	
				else if (rgtcol5km > col5km_end) {
					lftcol5km = col5km_end-1;	
					rgtcol5km = col5km_end;
				}
			
				lftcol = rcgrid[lftcol5km];	
				rgtcol = rcgrid[rgtcol5km];

				anglft = ang[irow * ncol + lftcol];
				angrgt = ang[irow * ncol + rgtcol];

				angint = anglft + (angrgt - anglft) * 1.0 / (rgtcol - lftcol) * (icol - lftcol);
				ang[irow * ncol + icol] = angint;
			}
		}
	}

	/* Second pass: linear interp for the pixels in all columns. All columns should have some non-fill data.
	 */
	for (icol = 0; icol < ncol; icol++) {
		row5km_start = -1;
		row5km_end = -1;
		for (irow5km = 0; irow5km < N5KM; irow5km++) {	 
			/* Find the range of rows which have the 5km data or the interpreted data from the first pass*/
			irow = rcgrid[irow5km];
			k = irow * ncol + icol; 
			if (ang[k] != ANGFILL && row5km_start == -1) 
				row5km_start = irow5km;
			 
			if (ang[k] != ANGFILL) 
				row5km_end = irow5km;
		}

		if (row5km_start == -1) {
			/*  Sep 7, 2016.
			 * This could happen when there is only one row in the tile that have the 5km values
			 * and the extrapolated value happen to be fill value for some columns that are 
			 * far from the detector's footprint.
			 *
			 * The extrapolated values for pixels far from the detector's footprint will not used anyway */
			/*
			sprintf(message, "No 5km or interp data  data on 0-based column %d", icol);
			Error(message);
			return(-1);
			*/
			continue;
		}
		else if (row5km_start == row5km_end) {  
			/* Only one value on this column; use it for all pixels on this column */
			tmprow = rcgrid[row5km_start];
			k5km = tmprow * ncol + icol;
			for (irow = 0; irow < nrow; irow++) 
				ang[irow * ncol + icol] = ang[k5km];
		} 
		else {
			search_start = 0;
			for (irow = 0; irow < nrow; irow++) {
				for (irow5km = search_start; irow5km < N5KM-1; irow5km++) {
					if (irow >= rcgrid[irow5km] && irow <= rcgrid[irow5km+1]) {
						toprow5km  = irow5km;
						botrow5km  = irow5km+1;
						search_start = irow5km;	/* From where the search start for next fine-reso pixel*/
						break;
					}
				}

				if (toprow5km < row5km_start) {
					toprow5km = row5km_start;
					botrow5km = row5km_start+1;
				}
				if (botrow5km > row5km_end) {
					toprow5km = row5km_end-1;
					botrow5km = row5km_end;
				}

				toprow = rcgrid[toprow5km];
				botrow = rcgrid[botrow5km];
				angtop = ang[toprow * ncol + icol];
				angbot = ang[botrow * ncol + icol];

				/* For locations far from the detector's footprint, the extrapolated values can be
				 * coincidentally fill value. This test would raise false alarm. 
				 * The extrapolated values for pixels far from the detector's footprint will not used anyway */
				/*
				if (angtop == ANGFILL || angbot == ANGFILL) {
					fprintf(stderr, "Somthing is wrong in interpreting the columns: second pass\n");
					fprintf(stderr, "icol, irow = %d, %d, toprow = %d, botrow = %d\n", icol, irow, toprow, botrow);
					fprintf(stderr, "top = %d, bot = %d\n", angtop, angbot);
					fprintf(stderr, "row5km_start, end = %d, %d\n\n", row5km_start, row5km_end);
				}
				*/
				angint = angtop + (angbot - angtop) * 1.0 / (botrow - toprow) * (irow - toprow);
				ang[irow * ncol + icol] = angint;
			}
		}
	}

	return 0;
}


void find_substitute(uint8 *angleavail, uint8 *subId)
{
	int ib, idx;

	for (ib = 0; ib < S2NBAND; ib++) {
		subId[ib] = ib;	/* In case there is no missing */

		if ( ! angleavail[ib]) {
			if (ib <= 9) {  /* The VNIR bands, in the same focal plane */
					/* Try in the VNIR bands first; SWIR is the last choice */ 
				for (idx = 0; idx < S2NBAND; idx++) {   
					if (angleavail[idx]) {
						subId[ib] = idx;
						break;
					}
				}
			}

			/* For SWIR bands, try SWIR bands first */
			if (ib > 9) {
				char found = 0;
				/* SWIR bands */
				for (idx = 10; idx < S2NBAND; idx++) {
					if (angleavail[idx]) {
						subId[ib] = idx;
						found = 1;
						break;
					}
				}

				if ( ! found) {
					/* VNIR focal plane */
					for (idx = 0; idx <= 9; idx++) {   
						if (angleavail[idx]) {
							subId[ib] = idx;
							break;
						}
					}
				}
			}
		}
	}

	return;
}

/* close */
int close_s2ang(s2ang_t *s2ang)
{
	char message[MSGLEN];
	int ib;

	if (s2ang->access_mode == DFACC_CREATE && s2ang->sd_id != FAIL) {
		char sdsname[500];     
		int32 start[2], edge[2];

		start[0] = 0; edge[0] = s2ang->nrow;
		start[1] = 0; edge[1] = s2ang->ncol;


		/* SZ */
		strcpy(sdsname, SZ);
		if (SDwritedata(s2ang->sds_id_sz, start, NULL, edge, s2ang->sz) == FAIL) {
			Error("Error in SDwritedata");
			return(ERR_CREATE);
		}
		SDendaccess(s2ang->sds_id_sz);

		/* SA */
		strcpy(sdsname, SA);
		if (SDwritedata(s2ang->sds_id_sa, start, NULL, edge, s2ang->sa) == FAIL) {
			Error("Error in SDwritedata");
			return(ERR_CREATE);
		}
		SDendaccess(s2ang->sds_id_sa);

		for (ib = 0; ib < S2NBAND; ib++) {
			/* VZ */
			sprintf(sdsname, "%s_%s", S2_SDS_NAME[ib], VZ);
			if (SDwritedata(s2ang->sds_id_vz[ib], start, NULL, edge, s2ang->vz[ib]) == FAIL) {
				Error("Error in SDwritedata");
				return(ERR_CREATE);
			}
			SDendaccess(s2ang->sds_id_vz[ib]);
	
			/* VA */
			sprintf(sdsname, "%s_%s", S2_SDS_NAME[ib], VA);
			if (SDwritedata(s2ang->sds_id_va[ib], start, NULL, edge, s2ang->va[ib]) == FAIL) {
				Error("Error in SDwritedata");
				return(ERR_CREATE);
			}
			SDendaccess(s2ang->sds_id_vz[ib]);
		}


		/* Write ANGLEAVAIL */
		SDsetattr(s2ang->sd_id, ANGLEAVAIL, DFNT_UINT8, S2NBAND, s2ang->angleavail);

		SDend(s2ang->sd_id);
		s2ang->sd_id = FAIL;
	}


	/* free up memory */
	if (s2ang->sz != NULL) {
		free(s2ang->sz);
		s2ang->sz = NULL;
	}

	if (s2ang->sa != NULL) {
		free(s2ang->sa);
		s2ang->sa = NULL;
	}

	for (ib = 0; ib < S2NBAND; ib++) {
		if (s2ang->vz[ib] != NULL) {
			free(s2ang->vz[ib]);
			s2ang->vz[ib] = NULL;
		}
	
		if (s2ang->va[ib] != NULL) {
			free(s2ang->va[ib]);
			s2ang->va[ib] = NULL;
		}
	}


	return 0;
}
