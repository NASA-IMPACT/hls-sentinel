#include "s2detfoo.h"
#include "error.h"

/********************************************************************************
* open S2 footprint for read or create
*/
int open_s2detfoo(s2detfoo_t *s2detfoo, intn access_mode)
{
	char sds_name[500];
	int32 sds_index;
	int32 sd_id, sds_id;
	int32 nattr, attr_index;
	//char attr_name[200];
	int32 dim_sizes[2];
	int32 rank, data_type, n_attrs;
	int32 start[2], edge[2];
	int ib;

	char message[MSGLEN];

	s2detfoo->access_mode = access_mode;
	for (ib = 0; ib < S2NBAND; ib++)
		s2detfoo->detid[ib] = NULL;

	/* For DFACC_READ, find the image dimension from band 1.
	 * For DFACC_CREATE, image dimension is given.
	 */
	if (s2detfoo->access_mode == DFACC_READ) {
		Error("The read function for detfoo is not implemented yet");
		return(2);
	}
	else if (s2detfoo->access_mode == DFACC_CREATE) {
		int irow, icol;
		char *dimnames[] = {"YDim_Grid", "XDim_Grid"};
		int32 comp_type;   /*Compression flag*/
		comp_info c_info;  /*Compression structure*/
		comp_type = COMP_CODE_DEFLATE;
		c_info.deflate.level = 2;     /*Level 9 would be too slow */
		rank = 2;

		if ((s2detfoo->sd_id = SDstart(s2detfoo->fname, s2detfoo->access_mode)) == FAIL) {
			sprintf(message, "Cannot create file: %s", s2detfoo->fname);
			Error(message);
			return(ERR_CREATE);
		}

		dim_sizes[0] = s2detfoo->nrow;
		dim_sizes[1] = s2detfoo->ncol;


		for (ib = 0; ib < S2NBAND; ib++) {
			sprintf(sds_name, "%s_detfoo", S2_SDS_NAME[ib]);

			if ((s2detfoo->detid[ib] = (uint8*)calloc(dim_sizes[0] * dim_sizes[1], sizeof(uint8))) == NULL) {
				Error("Cannot allocate memory");
				exit(1);
			}
			if ((s2detfoo->sds_id_detid[ib] = SDcreate(s2detfoo->sd_id, sds_name, DFNT_UINT8, rank, dim_sizes)) == FAIL) {
				sprintf(message, "Cannot create SDS %s", sds_name);
				Error(message);
				return(ERR_CREATE);
			}
			PutSDSDimInfo(s2detfoo->sds_id_detid[ib], dimnames[0], 0);
			PutSDSDimInfo(s2detfoo->sds_id_detid[ib], dimnames[1], 1);
			SDsetcompress(s2detfoo->sds_id_detid[ib], comp_type, &c_info);

			for (irow = 0; irow < s2detfoo->nrow; irow++) {
				for (icol = 0; icol < s2detfoo->ncol; icol++)
					s2detfoo->detid[ib][irow * s2detfoo->ncol + icol] = DETIDFILL;
			}
		}
	}


	return 0;
}

/********************************************************************************
 * Rasterize the footprint vector polygon and fill the polygon with detector id.
 * For the overlapped area,  fill with id_left * NDETECTOR + id_right (id_left and id_right are
 * the detector id on the left and right respectively.
 * Use the b01 gml filename to find the gml of all bands.
 *
 * Return 100 if the detfoo vector is not found.  May 1, 2017.
 */
int rasterize_s2detfoo(s2detfoo_t *s2detfoo, char *fname_b01_gml)
{
	char fname_gml[LINELEN];
	FILE *fgml;
	char line[500];
	char str[500];
	char *pos;
	int irow, icol;
	int detid;
	int xylen = 200; 	/* initial length of x,y vectors */
	double *x, *y, z;
	double px, py;
	int i, n, k;
	int ib;
	char found_vector; 	/* Indicator whether vector is present for a band */
	char message[MSGLEN];
	int no_vector = 100;    /* Return if no vector is found */

	pos = strstr(fname_b01_gml, "DETFOO"); /* Cannot be more specific because of different naming convention */
        if (pos == NULL) {
                sprintf(message, "Not gml file? %s", fname_b01_gml);
		Error(message);
		return(1);
        }

	if ((x = malloc(xylen *sizeof(double))) == NULL || (y = malloc(xylen *sizeof(double))) == NULL) {
                sprintf(message, "Cannot allocate memory");
		Error(message);
		return(1);
	}
	for (ib = 0; ib < S2NBAND; ib++) {
		strcpy(fname_gml, fname_b01_gml);
		pos = strstr(fname_gml, "B01"); /* Works for both naming conventions */
		strncpy(pos, S2_SDS_NAME[ib], 3);
		if ((fgml = fopen(fname_gml, "r")) == NULL) {
			sprintf(message, "Cannot open for read: %s", fname_gml);
			Error(message);
			return(1);
		}

		found_vector = 0;
		while (fgets(line, sizeof(line), fgml)) {
			 /* footprint extent can be smaller than tile extent. */

			/* One polygon (footprint for one detector module) */
			if (strstr(line, "<eop:MaskFeature gml:id=\"detector_footprint-B")) {

				/* Dec 6, 2016 
				 * Some flawed gml does not give the footprint vector at all. 
				 */
				found_vector = 1;
					
				/* Example: <eop:MaskFeature gml:id="detector_footprint-B8A-12-0">  */
				pos = strstr(line, "-");
				pos = strstr(pos+1, "-"); /* Skip band number since one band per gml file */
				detid = atoi(pos+1);

				/* Skip 5 lines and look for the expected characters*/
				for (i = 0; i < 5; i++) 
					fgets(line, sizeof(line), fgml);	
				/* Line of vector */
				fscanf(fgml, "%s", str);
				fscanf(fgml, "%s", str);
				if (strstr(str, "srsDimension=\"3\">") == NULL) {
					sprintf(message, "Pattern \"srsDimension\" not found. %s", fname_gml);
					Error(message);
					return(1);
				}

				n = 0;
				sscanf(str+strlen("srsDimension=\"3\">"), "%lf", &x[n]); /* First point */
				fscanf(fgml, "%lf", &y[n]);
				fscanf(fgml, "%lf", &z);
				n++;
				while (1) {
					fscanf(fgml, "%s", str);
					if (strstr(str, "</gml:posList>"))
						break;
					else {
						/* str contains x */
						if (n == xylen) {
							xylen *= 2;
							if ((x = realloc(x, xylen*sizeof(double))) == NULL || 
							    (y = realloc(y, xylen*sizeof(double))) == NULL) {
								sprintf(message, "Cannot open for read: %s", fname_gml);
								Error(message);
								return(1);
							}
						}

						x[n] = atof(str);
						fscanf(fgml, "%lf", &y[n]);
						fscanf(fgml, "%lf", &z);
						n++;
					}
				}

				/* Rasterize by point in polygon test */
				for (irow = 0; irow < s2detfoo->nrow; irow++) {
  					int i, j;
					int m = 0, im;
					char in; 	/* pixel in polygon or not */	
					double ex[100];	/* Expected x value on a polygon boundary for the given y. 
							 * Predomimantly only two elements are needed in the array. Nov 27, 2019 */
					py = s2detfoo->uly - (irow+0.5) * DETFOOPIXSZ;

					/* Dec 19-20, 2018: Originally ESA masked the full extent of each detector's
					 * footprint and as a result the footprint of two adjacent detectors overlaps 
					 * although data from only one detector is retained in the image for the overlap. 
					 * Later in extracting the angle data, HLS had evenly split the overlapping area.
					 * Starting on Nov 10 (?), 2018, the ESA footprint mask only demarcates the pixels
					 * that are preserved in the L1C data. As a result, the number of points in the mask
					 * increased substantially to make the application of the function pnpoly impractical.
					 *
					 * However, the idea of function pnpoly (found from internet) still applies. For each 
					 * row of pixels, the left and right boundaries of the footprint are calculated from 
					 * the mask vector and then each pixel's position is assessed with respect to the 
					 * boundaries.
					 *
					 * Nov 27, 2019: Bug fix. The footprint polygon is not convex at high latitude, where
					 * the flight path is almost horizontal in the image. So not necessarily only two 
					 * boundaries to test against for a row of pixels. Back to the original idea of ray
					 * tracing, but luckily all pixels in the same row have the same y, so the same set of 
					 * expected X on the boundaries is used, although the number of points in the set can
					 * be more than 2. 
					 */
  					for (i = 0, j = n-1; i < n; j = i++) {
						/* Note that the initial condition of the loop considers the first and last
						 * points in the vector in case the first point is not duplicated to be the
						 * last point.
						 */
    						if ( (y[i]>py) != (y[j]>py) ) {
							/* Very clever comparison to find a non-zero-length interval in y that
							 * encloses py; it does not matter which is greater, y[i] or y[j].
							 * py can be equal to one of the end points.
							 */
	 						ex[m] = (x[j]-x[i]) * (py-y[i]) / (y[j]-y[i]) + x[i];
							m++;

							/* m can be greater than 2. V1.4 code breaks the loop at m == 2 */ 
						}
					}

					/* No footprint on this row of pixels; right edge near the top on the tile.
					 * A bug fixed during testing. Dec 20, 2018 */
					if (m == 0)
						continue;

					for (icol = 0; icol < s2detfoo->ncol; icol++) {
						in = 0; 	/* Not in the polygon*/
						px = s2detfoo->ulx + (icol+0.5) * DETFOOPIXSZ;
						k = irow * s2detfoo->ncol + icol;
					
						/* The pixel hasn't been flagged.
						 * Nov 27, 2019: no footprint overlap any more. 
						 */
						if (s2detfoo->detid[ib][k] == DETIDFILL) {
							for (im = 0; im < m; im++) {
								if (px < ex[im])	/* borrowed from pnpoly.c. 11/27/19 */
									in = !in;
							}

							if ( in ) 
								s2detfoo->detid[ib][k] = detid;
							
						}
					}
				}
			}
		}

		if (! found_vector) {
			sprintf(message, "vector not found for band %s: %s", S2_SDS_NAME[ib], fname_gml);
			Error(message);	
			return(no_vector);
		}

		fclose(fgml);
	}
	return 0;
}

/***********************************************************************
 * Equally split the overlap area for each row of image.
 * 
 * Dec 20, 2018: The footprint of new data (baseline 02.07, starting early November 2018) 
 * no longer overlaps between two adjacent detectors. As a result this code does nothing
 * for baseline 02.07 onwards but is kept for earlier data. 
 * 
 * Change on Dec 20, 2018: For the majority of the rows and interior of the rows, the overlap
 * width can be measured locally. If the overlap extends to the left or right edge of the tile, 
 * the overlap width is unknown locally and use an estimated overlap width from the interior
 * of the first and the last rows of the image.  If an estimation is impossible, use a prefixed 
 * overlap width OW.
 */
void split_overlap(s2detfoo_t *s2detfoo)
{
	int irow, icol, k;
	int startcol, endcol;
	int id_left, id_right, id_overlap;
	int ic, mid, n;
	int ib;
	int OW = 50; 	/* An priori estimate of overlap width; not accurate for all detector pairs
			 * and for all bands. Later if possible, a more suitable value will be found
			 * from the overlap in the footprint image itself.  Added Dec 20, 2018 */


	for (ib = 0; ib < S2NBAND; ib++) {

		/* Try to find a more realistic value for OW from the full-length overlap in the interior 
		 * of the first and last rows of the footprint image. Do an average. Still this value 
		 * won't be perfect.  The attempt may fail if the footprint image is too small to contain a 
		 * full-length overlap; use a prefixed OW in this case.
		 */
		int n = 0;
		double ow = 0;
		for (irow = 0;        ; irow = s2detfoo->nrow -1) {   /* The first and last rows */
			startcol = endcol = -1;
			for (icol = 0; icol < s2detfoo->ncol; icol++) {
				k = irow * s2detfoo->ncol + icol;

 				/* A full-length overlap shouldn't start with column 0. */
				if ( s2detfoo->detid[ib][k] > NDETECTOR && startcol == -1 && icol != 0 )
					startcol = icol;
					
 				/* A full-length overlap shouldn't end with column ncol-1. */
				if (s2detfoo->detid[ib][k] <= NDETECTOR && startcol != -1 && icol != s2detfoo->ncol - 1)
					endcol = icol-1;

				if (startcol != -1 && endcol != -1) {
					n++;
					ow = ow + ((endcol - startcol + 1) - ow)/n;

					/* Initialize for the next overlap on this row. */
					startcol = endcol = -1;
				}
			}

			if (irow == s2detfoo->nrow -1) {
				if (ow != 0) 
					OW = ow;

				break;
			}
		}


		/* Now start to split on each row */
		for (irow = 0; irow < s2detfoo->nrow; irow++) {
			startcol = endcol = -1;
			for (icol = 0; icol < s2detfoo->ncol; icol++) {
				k = irow * s2detfoo->ncol + icol;

				if (s2detfoo->detid[ib][k] > NDETECTOR) {  /* In overlap */
					if (startcol == -1) {
						startcol = icol;
						id_overlap = s2detfoo->detid[ib][k];
					}

					/* Has reached the end of the line and still in overlap */
					/* New note on Dec 20, 2018: Without looking at the adjacent tile on the 
				 	 * right we can never reliably split the overlap. That is why the new, 
					 * baseline 2.07 is better.  */
					if (icol == s2detfoo->ncol-1)
						endcol = icol;
				}

				/* Overlap stopped in the preceding column */
				if (s2detfoo->detid[ib][k] <= NDETECTOR && startcol != -1)
					endcol = icol-1;


				/* Split. The processing baseline 2.07 onwards simply skips this loop.  
				 * If only a few pixels on the swath edge appear in this tile for data prior to 2.07,
				 * this loop might be skipped as well.
				 */
				if (startcol != -1 && endcol != -1) {
					id_left = (id_overlap -1)/(NDETECTOR+1);	/* id_left * NDETECTOR + (id_left+1) = id_overlap */
					id_right = id_left + 1;		/* Detect ID to the right is 1 greater than on the left. */

					if (startcol == 0) {
						/* Don't know where the full-length overlap starts */
						mid = endcol - OW/2;	
						if (mid < 0)
							mid = 0;
					}
					else if (endcol == s2detfoo->ncol-1) { 
						/* Don't know where the full-length overlap ends */
						mid = startcol + OW/2;	
						if (mid > s2detfoo->ncol-1)
							mid = s2detfoo->ncol-1;
					}
					else 
						mid = (endcol+startcol)/2;

					for (ic = startcol; ic < mid; ic++) {
						n = irow * s2detfoo->ncol + ic;
						s2detfoo->detid[ib][n] = id_left;
					}
					for (ic = mid; ic <= endcol; ic++) {
						n = irow * s2detfoo->ncol + ic;
						s2detfoo->detid[ib][n] = id_right;
					}

					/* Initialize for the next overlap on this row. */
					startcol = endcol = -1;
				}
			}
		}
	}
}


/********************************************************************************
 * close 
 */
int close_s2detfoo(s2detfoo_t *s2detfoo)
{
	int ib;
	char message[MSGLEN];

	if (s2detfoo->access_mode == DFACC_CREATE && s2detfoo->sd_id != FAIL) {
		char sds_name[500];
		int32 start[2], edge[2];

		start[0] = 0; edge[0] = s2detfoo->nrow;
		start[1] = 0; edge[1] = s2detfoo->ncol;


		for (ib = 0; ib < S2NBAND; ib++) {
			if (SDwritedata(s2detfoo->sds_id_detid[ib], start, NULL, edge, s2detfoo->detid[ib]) == FAIL) {
				Error("Error in SDwritedata");
				return(ERR_CREATE);
			}
			SDendaccess(s2detfoo->sds_id_detid[ib]);
		}
		SDend(s2detfoo->sd_id);
	}


	/* free up memory */
	for (ib = 0; ib < S2NBAND; ib++) {
		if (s2detfoo->detid[ib] != NULL) {
			free(s2detfoo->detid[ib]);
			s2detfoo->detid[ib] = NULL;
		}
	}

	return 0;
}
