#include "s2at30m.h" 
#include "util.h"

int open_s2at30m(s2at30m_t *s2at30m, intn access_mode) 
{
	int ib;	/* the index of one of the 13 bands in the wavelength order. */
	char message[MSGLEN];

	/*** Create the output file */
	char sds_name[500];
	char *dimnames[] = {"YDim_Grid", "XDim_Grid"};
	int32 rank, data_type, n_attrs;
	int32 dimsizes[2];
	int32 start[2], edge[2];
	int32 sd_id, sds_id;
	int32 sds_index;
	int32 nattr, attr_index;
	char attr_name[500];
	int32 count;
	int32 comp_type;   /*Compression flag*/
	comp_info c_info;  /*Compression structure*/
	comp_type = COMP_CODE_DEFLATE;
	c_info.deflate.level = 2;     /*Level 9 would be too slow */
	rank = 2;

	s2at30m->access_mode = access_mode;
	
	s2at30m->sd_id = FAIL;
	for (ib = 0; ib < S2NBAND; ib++) { 
		s2at30m->ref[ib] = NULL;
		s2at30m->sds_id_ref[ib] = FAIL;
	}
	s2at30m->acmask = NULL;
	s2at30m->fmask = NULL;

	/* Allocate memory for either READ or CREATE.
	 * When READ, find the dimension from input file; 
	 * when CREATE, dimension is directly given.
	 */
	if (s2at30m->access_mode == DFACC_READ || s2at30m->access_mode == DFACC_WRITE) {
		if ((sd_id = SDstart(s2at30m->fname, s2at30m->access_mode)) == FAIL) {
			sprintf(message, "Cannot open for read: %s", s2at30m->fname);
			Error(message);
			return(ERR_READ);
		}

		strcpy(sds_name, S2_SDS_NAME[0]); /* Any other band will do too. */
		if ((sds_index = SDnametoindex(sd_id, sds_name)) == FAIL) {
			sprintf(message, "Didn't find the SDS %s in %s", sds_name, s2at30m->fname);
			Error(message);
			return(ERR_READ);
		}
		sds_id = SDselect(sd_id, sds_index);
		if (SDgetinfo(sds_id, sds_name, &rank, dimsizes, &data_type, &nattr) == FAIL) {
			Error("Error in SDgetinfo");
			return(ERR_READ);
		} 
		SDendaccess(sds_id);
		SDend(sd_id);

		s2at30m->nrow = dimsizes[0];
		s2at30m->ncol = dimsizes[1];
	}
	else if (s2at30m->access_mode == DFACC_CREATE){
		dimsizes[0] = s2at30m->nrow; 	/* Given */
 		dimsizes[1] = s2at30m->ncol;
	}
	else {
		sprintf(message, "Access mode not considered: %d for %s", DFACC_CREATE, s2at30m->fname);
		Error(message);
		return(1);
	}

	/* Memory for reflectance */
	for (ib = 0; ib < S2NBAND; ib++) {
		if ((s2at30m->ref[ib] = (int16*)calloc(dimsizes[0] * dimsizes[1], sizeof(int16))) == NULL) {
			Error("Cannot allocate memory");
			return(1);
		}
	}
	/* ACmask and Fmask */
	if ((s2at30m->acmask = (uint8*)calloc(dimsizes[0] * dimsizes[1], sizeof(uint8))) == NULL) {
		Error("Cannot allocate memory");
		return(1);
	}
	if ((s2at30m->fmask = (uint8*)calloc(dimsizes[0] * dimsizes[1], sizeof(uint8))) == NULL) {
		Error("Cannot allocate memory");
		return(1);
	}

	/* Now READ or CREATE */
	if (s2at30m->access_mode == DFACC_READ || s2at30m->access_mode == DFACC_WRITE) {
		if ((s2at30m->sd_id = SDstart(s2at30m->fname, s2at30m->access_mode)) == FAIL) {
			sprintf(message, "Cannot open for read %s", s2at30m->fname);
			Error(message);
			return(ERR_READ);
		}

		start[0] = 0; edge[0] = dimsizes[0];
		start[1] = 0; edge[1] = dimsizes[1];
		for (ib = 0; ib < S2NBAND; ib++) {
			strcpy(sds_name, S2_SDS_NAME[ib]);
			if ((sds_index = SDnametoindex(s2at30m->sd_id, sds_name)) == FAIL) {
				sprintf(message, "Didn't find the SDS %s in %s", sds_name, s2at30m->fname);
				Error(message);
				return(ERR_READ);
			}
			s2at30m->sds_id_ref[ib] = SDselect(s2at30m->sd_id, sds_index);

			if (SDreaddata(s2at30m->sds_id_ref[ib], start, NULL, edge, s2at30m->ref[ib]) == FAIL) {
				sprintf(message, "Error reading sds %s in %s", sds_name, s2at30m->fname);
				Error(message);
				return(ERR_READ);
			}
		}

		/*ACmask */
		strcpy(sds_name, ACMASK_NAME);
		if ((sds_index = SDnametoindex(s2at30m->sd_id, sds_name)) == FAIL) {
			sprintf(message, "Didn't find the SDS %s in %s", sds_name, s2at30m->fname);
			Error(message);
			return(ERR_READ);
		}
		s2at30m->sds_id_acmask = SDselect(s2at30m->sd_id, sds_index);

		if (SDreaddata(s2at30m->sds_id_acmask, start, NULL, edge, s2at30m->acmask) == FAIL) {
			sprintf(message, "Error reading sds %s in %s", sds_name, s2at30m->fname);
			Error(message);
			return(ERR_READ);
		}

		/*Fmask */
		strcpy(sds_name, FMASK_NAME);
		if ((sds_index = SDnametoindex(s2at30m->sd_id, sds_name)) == FAIL) {
			sprintf(message, "Didn't find the SDS %s in %s", sds_name, s2at30m->fname);
			Error(message);
			return(ERR_READ);
		}
		s2at30m->sds_id_fmask = SDselect(s2at30m->sd_id, sds_index);

		if (SDreaddata(s2at30m->sds_id_fmask, start, NULL, edge, s2at30m->fmask) == FAIL) {
			sprintf(message, "Error reading sds %s in %s", sds_name, s2at30m->fname);
			Error(message);
			return(ERR_READ);
		}
		
		/*** Read ULX, ULY, zonehem ***/
		{
			//read_envi_utm_header(header, s2at30m->zonehem, &s2at30m->ulx, &s2at30m->uly);
			/* Nov 17, 2016: The same snippet has been implemented in s2r. Need to turn this 
			 * into a function to be used by both.
			 */
			
			char cs_name[100], *cpos;
	
			/* ULX */
			strcpy(attr_name, ULX);
			if ((attr_index = SDfindattr(s2at30m->sd_id, attr_name)) == FAIL) {
				sprintf(message, "Attribute \"%s\" not found in %s", attr_name, s2at30m->fname);
				Error(message);
				return (-1);
			}
			if (SDreadattr(s2at30m->sd_id, attr_index, &s2at30m->ulx) == FAIL) {
				sprintf(message, "Error read attribute \"%s\" in %s", attr_name, s2at30m->fname);
				Error(message);
				return(-1);
			}
		
			/* ULY */
			strcpy(attr_name, ULY);
			if ((attr_index = SDfindattr(s2at30m->sd_id, attr_name)) == FAIL) {
				sprintf(message, "Attribute \"%s\" not found in %s", attr_name, s2at30m->fname);
				Error(message);
				return (-1);
			}
			if (SDreadattr(s2at30m->sd_id, attr_index, &s2at30m->uly) == FAIL) {
				sprintf(message, "Error read attribute \"%s\" in %s", attr_name, s2at30m->fname);
				Error(message);
				return(-1);
			}
		
			/* HORIZONTAL_CS_NAME*/
			/* Sentinel example:  "WGS84 / UTM zone 34S" 
			 *
			 * On the Landsat side,  this information is in slightly different format:  "UTM, WGS84, UTM ZONE 29" 
			 * Should have used the same format. Nov 17, 2016.
			 */
			strcpy(attr_name, HORIZONTAL_CS_NAME);
			if ((attr_index = SDfindattr(s2at30m->sd_id, attr_name)) == FAIL) {
				sprintf(message, "Attribute \"%s\" not found in %s", attr_name, s2at30m->fname);
				Error(message);
				return (-1);
			}
			if (SDattrinfo(s2at30m->sd_id, attr_index, attr_name, &data_type, &count) == FAIL) {
				Error("Error in SDattrinfo");
				return(-1);
			}
			if (SDreadattr(s2at30m->sd_id, attr_index, cs_name) == FAIL) {
				sprintf(message, "Error read attribute \"%s\" in %s", attr_name, s2at30m->fname);
				Error(message);
				return(-1);
			}
			cs_name[count] = '\0';
			cpos = strrchr(cs_name, ' ');
			strcpy(s2at30m->zonehem, cpos+1);


			/* Added on Oct 4, 2018. To GCTP convention. This is only a remedy. In future process, this should
			 * have been adjusted earlier in S10 processing file s2r.c.
			 */
			if (strstr(s2at30m->zonehem, "S") && s2at30m->uly > 0) {
					s2at30m->uly -= pow(10,7);

					if (s2at30m->access_mode == DFACC_WRITE) {
						SDsetattr(s2at30m->sd_id, ULX, DFNT_FLOAT64, 1, (VOIDP)&(s2at30m->ulx));
						SDsetattr(s2at30m->sd_id, ULY, DFNT_FLOAT64, 1, (VOIDP)&(s2at30m->uly));
					}
			}
		}

	}
	else if (s2at30m->access_mode == DFACC_CREATE) {
		int irow, icol;
		if ((s2at30m->sd_id = SDstart(s2at30m->fname, DFACC_CREATE)) == FAIL) {
			sprintf(message, "Cannot create %s", s2at30m->fname);
			Error(message);
			return(ERR_CREATE);
		}
	
		for (ib = 0; ib < S2NBAND; ib++) {
			strcpy(sds_name, S2_SDS_NAME[ib]);
			if ((s2at30m->sds_id_ref[ib] = SDcreate(s2at30m->sd_id, sds_name, DFNT_INT16, rank, dimsizes)) == FAIL) {
				sprintf(message, "Cannot create SDS %s", sds_name);
				Error(message);
				return(ERR_CREATE);
			}    
			PutSDSDimInfo(s2at30m->sds_id_ref[ib], dimnames[0], 0);
			PutSDSDimInfo(s2at30m->sds_id_ref[ib], dimnames[1], 1);
			SDsetcompress(s2at30m->sds_id_ref[ib], comp_type, &c_info);	
      			SDsetattr(s2at30m->sds_id_ref[ib], "long_name", DFNT_CHAR8, 
							strlen(S2_SDS_LONG_NAME[ib]), (VOIDP)S2_SDS_LONG_NAME[ib]);
                        SDsetattr(s2at30m->sds_id_ref[ib], "_FillValue", DFNT_CHAR8, 
							strlen(S2_ref_fillval), (VOIDP)S2_ref_fillval);
                        SDsetattr(s2at30m->sds_id_ref[ib], "scale_factor", DFNT_CHAR8, 
                                                        strlen(S2_ref_scale_factor), (VOIDP)S2_ref_scale_factor);
                        SDsetattr(s2at30m->sds_id_ref[ib], "add_offset", DFNT_CHAR8, 
                                                        strlen(S2_ref_add_offset), (VOIDP)S2_ref_add_offset);
			for (irow = 0; irow < s2at30m->nrow; irow++) {
				for (icol = 0; icol < s2at30m->ncol; icol++) 
					s2at30m->ref[ib][irow * s2at30m->ncol+icol] = ref_fillval;
			}
		}

		/***ACmask ***/
		strcpy(sds_name, ACMASK_NAME);
		if ((s2at30m->sds_id_acmask = SDcreate(s2at30m->sd_id, sds_name, DFNT_UINT8, rank, dimsizes)) == FAIL) {
			sprintf(message, "Cannot create SDS %s", sds_name);
			Error(message);
			return(ERR_CREATE);
		}
		PutSDSDimInfo(s2at30m->sds_id_acmask, dimnames[0], 0);
		PutSDSDimInfo(s2at30m->sds_id_acmask, dimnames[1], 1);
		SDsetcompress(s2at30m->sds_id_acmask, comp_type, &c_info);	
		SDsetattr(s2at30m->sds_id_acmask, "_FillValue", DFNT_UINT8, 1, (VOIDP)&S2_mask_fillval);

		char attr[3000];
		/* Note: For better view, the blanks within the string is blank space characters, not tab */
		sprintf(attr, 	"Bits are listed from the MSB (bit 7) to the LSB (bit 0): \n"
				"7-6    aerosol:\n"
				"       00 - climatology\n"
				"       01 - low\n"
				"       10 - average\n"
				"       11 - high\n"
				"5      water\n"
				"4      snow/ice\n"
				"3      cloud shadow\n"
				"2      adjacent to cloud\n"
				"1      cloud\n"
				"0      cirrus cloud\n");
		SDsetattr(s2at30m->sds_id_acmask, "ACmask description", DFNT_CHAR8, strlen(attr), (VOIDP)attr);

		for (irow = 0; irow < s2at30m->nrow; irow++) {
			for (icol = 0; icol < s2at30m->ncol; icol++) 
				s2at30m->acmask[irow * s2at30m->ncol+icol] = S2_mask_fillval; 
		}

		/***Fmask ***/
		strcpy(sds_name, FMASK_NAME);
		if ((s2at30m->sds_id_fmask = SDcreate(s2at30m->sd_id, sds_name, DFNT_UINT8, rank, dimsizes)) == FAIL) {
			sprintf(message, "Cannot create SDS %s", sds_name);
			Error(message);
			return(ERR_CREATE);
		}
		PutSDSDimInfo(s2at30m->sds_id_fmask, dimnames[0], 0);
		PutSDSDimInfo(s2at30m->sds_id_fmask, dimnames[1], 1);
		SDsetcompress(s2at30m->sds_id_fmask, comp_type, &c_info);	
		SDsetattr(s2at30m->sds_id_fmask, "_FillValue", DFNT_UINT8, 1, (VOIDP)&S2_mask_fillval);

		/* Note: For better view, the blanks within the string is blank space characters, not tab */
		sprintf(attr, 	"Bits are listed from the MSB (bit 7) to the LSB (bit 0): \n"
				"7-6    aerosol:\n"
				"       00 - climatology\n"
				"       01 - low\n"
				"       10 - average\n"
				"       11 - high\n"
				"5      water\n"
				"4      snow/ice\n"
				"3      cloud shadow\n"
				"2      adjacent to cloud\n"
				"1      cloud\n"
				"0      cirrus cloud\n");
		SDsetattr(s2at30m->sds_id_fmask, "Fmask description", DFNT_CHAR8, strlen(attr), (VOIDP)attr);

		for (irow = 0; irow < s2at30m->nrow; irow++) {
			for (icol = 0; icol < s2at30m->ncol; icol++) 	
				s2at30m->acmask[irow * s2at30m->ncol+icol] = S2_mask_fillval; 
		

		}
	}

	return 0;
}

int resample_s2to30m(s2r_t *s2r, s2at30m_t *s2at30m) 
{
	int irow, icol;
	int ib10m, ib20m, ib60m;
	int irow10m, icol10m, rstart10m, rend10m, cstart10m, cend10m;
	double ave;
	int n, k10m, k20m, k30m, k60m;
	char message[MSGLEN];

	unsigned char anyfill;	/* If any pixel used in aggregation is fill, output is fill */

	int ib;		
	/* The array index of a band in the increasing wavelength order. 
	 * band ID from s2r.h:
	 * static int S2bandID[] =  {1, 2, 3, 4, 5, 6, 7, 8, 80, 9, 10, 11,12};
	 */

	/*** Start to resample */
	
	/* Nov 14, 2017: if any reflectance or QA in the 3x3 window is fill, the output is fill */


	/* 10m to 30m, box-car average*/
	int boxsz = 3;
	for (ib10m = 0; ib10m < S2NB10M; ib10m++) {
		switch (ib10m) {
			case 0: ib = 1; break;	/* blue */
			case 1: ib = 2; break;	/* green*/
			case 2: ib = 3; break;	/* red */
			case 3: ib = 7; break;	/* nir broad*/
		}

		for (irow = 0; irow < s2at30m->nrow; irow++) { 
			rstart10m = irow * boxsz; 
			rend10m   = rstart10m + boxsz - 1;
			for (icol = 0; icol < s2at30m->ncol; icol++) { 
				cstart10m = icol * boxsz; 
				cend10m   = cstart10m + boxsz - 1;

				ave = 0;
				n = 0;
				anyfill = 0;
				for (irow10m = rstart10m; irow10m <= rend10m; irow10m++) {
					if (anyfill)
						break;
					for (icol10m = cstart10m; icol10m <= cend10m; icol10m++) {
						k10m = irow10m * s2r->ncol[0] + icol10m;
						if (s2r->ref[ib][k10m] == HLS_S2_FILLVAL) {
							anyfill = 1;
							break;
						}

						/* If not fill */
						n++;
						ave = ave + (s2r->ref[ib][k10m] - ave)/n;
					}
				}
				if (anyfill) /* s2at30m has been initialized to the fillval any way */
					s2at30m->ref[ib][irow*s2at30m->ncol+icol] = HLS_S2_FILLVAL;
				else
					s2at30m->ref[ib][irow*s2at30m->ncol+icol] = asInt16(ave);
			}
		}
	} 

	/* 20m to 30m, area weighted average. A 30m pixel overlaps with four
	 * 20m pixels, with area fractions 1, 0.5, 0.5, and 0.25 depending on the
	 * phase of the 30m pixel relative the 20m pixel; these area fractions
	 * are used as weights.
	 */
	int irow20m, icol20m;
	double tw = 1 + 0.5 + 0.5 + 0.25;	/* total weight */
	double w[2][2];	 /* Weights in the 2x2 window */
	int r0, r1, c0, c1; /* rows and columns of the 20m pixels in the 2x2 window */

	for (irow = 0; irow < s2at30m->nrow; irow++) {
		r0 = floor(irow * 30.0 / 20);  /* row indices of overlapping 20m pixels */
		r1 = r0+1;
		for (icol = 0; icol < s2at30m->ncol; icol++) {
			c0 = floor(icol * 30.0 / 20);  /* column indices of overlapping 20m pixels */
			c1 = c0 + 1;

			if (irow % 2 == 0) {
				if (icol % 2 == 0) {
					w[0][0] = 1.0;  w[0][1] = 0.5;
					w[1][0] = 0.5;  w[1][1] = 0.25; 
				}
				else {
					w[0][0] = 0.5;  w[0][1] = 1.0;
					w[1][0] = 0.25; w[1][1] = 0.5; 
				}
			}
			else {
				if (icol % 2 == 0) {
					w[0][0] = 0.5;  w[0][1] = 0.25;
					w[1][0] = 1.0;  w[1][1] = 0.5; 
				}
				else {
					w[0][0] = 0.25; w[0][1] = 0.5;
					w[1][0] = 0.5;  w[1][1] = 1.0; 
				}
			}

	
			k30m = irow * s2at30m->ncol + icol;
			for (ib20m = 0; ib20m < S2NB20M; ib20m++) {
				switch (ib20m) {
					case 0: ib = 4;  break;	/* Red edge short */
					case 1: ib = 5;  break; /* Red edge mid */
					case 2: ib = 6;  break; /* red edge long */
					case 3: ib = 8;  break;	/* 8a */
					case 4: ib = 11; break;
					case 5: ib = 12; break;
				}
	
				anyfill = 0;
				ave = 0;
				for (irow20m = r0; irow20m <= r1; irow20m++) {
					if (anyfill)
						break;
					for (icol20m = c0; icol20m <= c1; icol20m++) {
						k20m = irow20m * s2r->ncol[1] + icol20m;
						if (s2r->ref[ib][k20m] == HLS_S2_FILLVAL) {
							anyfill = 1;
							break;
						}

						ave += s2r->ref[ib][k20m] * w[irow20m-r0][icol20m-c0]; 
					}
				}
				if (anyfill) 	
					s2at30m->ref[ib][k30m] = HLS_S2_FILLVAL;
				else {
					ave = ave / tw;
					s2at30m->ref[ib][k30m] = asInt16(ave);
				}
			}
		}
	}

	/* 60m to 30m */
	for (ib60m = 0; ib60m < S2NB60M; ib60m++) {
		switch (ib60m) {
			case 0: ib = 0;  break;	   /* Aerosol */
			case 1: ib = 9;  break;	   /* Water vapor */
			case 2: ib = 10;  break;   /* Cirrus */
		}

		for (irow = 0; irow < s2at30m->nrow; irow++) {
			for (icol = 0; icol < s2at30m->ncol; icol++) {
				k30m = irow * s2at30m->ncol + icol;
				k60m = (irow/2) * s2r->ncol[2] + icol/2;
				s2at30m->ref[ib][k30m] = s2r->ref[ib][k60m];
			}
		}
	}

	/* ACmask and Fmask, from 10m to 30m.
	 * If any 10m mask vlue bit is set, the output 30m bit will be set.
	 * This rule applies to both the single-bit masks (bits 0-5) and the
	 * bits 6-7 as a group.
	 */
	int bit, i;
	int aerocnt[4];	 /* indicator of presense of qualitative four aerosol levels in 3x3 window */
	unsigned char mask, val;
	unsigned char anyset;	/* Any bit is set in the 3x3 window. */
	for (irow = 0; irow < s2at30m->nrow; irow++) {
		for (icol = 0; icol < s2at30m->ncol; icol++) {
			k30m = irow * s2at30m->ncol + icol;

			/********** ACmask **********/
			anyset = 0;
			for (irow10m = irow*3; irow10m < (irow+1)*3; irow10m++) {
				if (anyset)
					break;
				for (icol10m = icol*3; icol10m < (icol+1)*3; icol10m++) {
					k10m = irow10m * s2r->ncol[0] + icol10m;
					if (s2r->acmask[k10m] == S2_mask_fillval) {
						anyset = 1;
						break;
					}
				}
			}

			if (anyset) { 
				s2at30m->acmask[k30m] = S2_mask_fillval;
			}
			else {
				mask = 0;
				/* Bits 0-5 are individual bits. */
				for (bit = 0; bit <= 5; bit++) {
					/* The QA in 3 x 3 window of 10m pixels*/
					anyset = 0;	/* QA for any 10m pixel has fill value */
					for (irow10m = irow*3; irow10m < (irow+1)*3; irow10m++) {
						if (anyset)
							break;
						for (icol10m = icol*3; icol10m < (icol+1)*3; icol10m++) {
							k10m = irow10m * s2r->ncol[0] + icol10m;
							if ((s2r->acmask[k10m] >> bit) & 01) {
								anyset = 1;  
								break;
							}
						}
					}
					
					if (anyset) 
						mask = (mask | (01 << bit));
				}
	
				/* bits 6-7 are a group for qualitative aerosol; 4 levels. Oct 4, 2017 */
				aerocnt[0] = aerocnt[1] = aerocnt[2] = aerocnt[3] = 0;
				for (irow10m = irow*3; irow10m < (irow+1)*3; irow10m++) {
					for (icol10m = icol*3; icol10m < (icol+1)*3; icol10m++) {
						k10m = irow10m * s2r->ncol[0] + icol10m;
						val = ((s2r->acmask[k10m] >> 6) & 03); 
						aerocnt[val]++;
					}
				}
				/* A higher aerosol level takes precedence over a lower level. Nov 14, 2017 */
				val = 0;
				for (i = 0; i <= 3; i++) {  /* 4 levels */
					if (aerocnt[i] > 0)  
						val = i;
				}
				mask = (mask | (val << 6));
	
				/* Finally record the mask. */
				s2at30m->acmask[k30m] = mask;
			}


			/********** Fmask **********/
			anyset = 0;
			for (irow10m = irow*3; irow10m < (irow+1)*3; irow10m++) {
				if (anyset)
					break;
				for (icol10m = icol*3; icol10m < (icol+1)*3; icol10m++) {
					k10m = irow10m * s2r->ncol[0] + icol10m;
					if (s2r->fmask[k10m] == S2_mask_fillval) {
						anyset = 1;
						break;
					}
				}
			}

			if (anyset) { 
				s2at30m->fmask[k30m] = S2_mask_fillval;
			}
			else {
				mask = 0;
				/* Bits 0-5 are individual bits. */
				for (bit = 0; bit <= 5; bit++) {
					/* The QA in 3 x 3 window of 10m pixels*/
					anyset = 0;	/* QA for any 10m pixel has fill value */
					for (irow10m = irow*3; irow10m < (irow+1)*3; irow10m++) {
						if (anyset)
							break;
						for (icol10m = icol*3; icol10m < (icol+1)*3; icol10m++) {
							k10m = irow10m * s2r->ncol[0] + icol10m;
							if ((s2r->fmask[k10m] >> bit) & 01) {
								anyset = 1;  
								break;
							}
						}
					}
					
					if (anyset) 
						mask = (mask | (01 << bit));
				}
				/* Like ACmask, bits 6-7 are a group for qualitative aerosol level borrowed from USGS.
				 * 4 levels. May 11, 2021 */
				aerocnt[0] = aerocnt[1] = aerocnt[2] = aerocnt[3] = 0;
				for (irow10m = irow*3; irow10m < (irow+1)*3; irow10m++) {
					for (icol10m = icol*3; icol10m < (icol+1)*3; icol10m++) {
						k10m = irow10m * s2r->ncol[0] + icol10m;
						val = ((s2r->fmask[k10m] >> 6) & 03); 
						aerocnt[val]++;
					}
				}
				/* A higher aerosol level takes precedence over a lower level. Nov 14, 2017 */
				val = 0;
				for (i = 0; i <= 3; i++) {  /* 4 levels */
					if (aerocnt[i] > 0)  
						val = i;
				}
				mask = (mask | (val << 6));

				s2at30m->fmask[k30m] = mask;
			}
		}
	}

	s2at30m->tile_has_data = 1;

	return 0;
}

void dup_s2at30m(s2at30m_t *in, s2at30m_t *out)
{
	int ib;
	int k;

	/* Needed for map projection */
	strcpy(out->zonehem, in->zonehem);
	out->ulx = in->ulx;
	out->uly = in->uly;

	for (ib = 0; ib < S2NBAND; ib++) {
		for (k = 0; k < in->nrow * in->ncol; k++)
			out->ref[ib][k] = in->ref[ib][k];
	}
	for (k = 0; k < in->nrow * in->ncol; k++) {
		out->acmask[k] = in->acmask[k];
		out->fmask[k] =  in->fmask[k];
	}
}

/* close */
int close_s2at30m(s2at30m_t *s2at30m)
{
	int32 start[2], edge[2];
	int ib;
	char message[MSGLEN];

	if ((s2at30m->access_mode == DFACC_CREATE || s2at30m->access_mode == DFACC_WRITE) && s2at30m->sd_id != FAIL) {
		start[0] = 0; edge[0] = s2at30m->nrow;
		start[1] = 0; edge[1] = s2at30m->ncol;
		/* Reflectance */
		for (ib = 0; ib < S2NBAND; ib++) {
			if (SDwritedata(s2at30m->sds_id_ref[ib], start, NULL, edge, s2at30m->ref[ib]) == FAIL) {
				Error("Error in SDwritedata");
				return(ERR_CREATE);
			}
			SDendaccess(s2at30m->sds_id_ref[ib]);
		}

		/* ACMASK */
		if (SDwritedata(s2at30m->sds_id_acmask, start, NULL, edge, s2at30m->acmask) == FAIL) {
			Error("Error in SDwritedata");
			return(ERR_CREATE);
		}
		SDendaccess(s2at30m->sds_id_acmask);

		/* FMASK */
		if (SDwritedata(s2at30m->sds_id_fmask, start, NULL, edge, s2at30m->fmask) == FAIL) {
			Error("Error in SDwritedata");
			return(ERR_CREATE);
		}
		SDendaccess(s2at30m->sds_id_fmask);

		SDend(s2at30m->sd_id);
		s2at30m->sd_id = FAIL;

		/* Add an ENVI header*/
		char fname_hdr[500];
		sprintf(fname_hdr, "%s.hdr", s2at30m->fname);
		add_envi_utm_header(s2at30m->zonehem, s2at30m->ulx, s2at30m->uly, s2at30m->nrow, s2at30m->ncol, HLS_PIXSZ, fname_hdr);
	}

	if (s2at30m->access_mode == DFACC_READ && s2at30m->sd_id != FAIL) {
		for (ib = 0; ib < S2NBAND; ib++) 
			SDendaccess(s2at30m->sds_id_ref[ib]);

		SDend(s2at30m->sd_id);
		s2at30m->sd_id = FAIL;
	}


	/* free up memory */
	for (ib = 0; ib < S2NBAND; ib++) {
		if (s2at30m->ref[ib] != NULL) {
			free(s2at30m->ref[ib]);
			s2at30m->ref[ib] = NULL;
		}
	}
	if (s2at30m->acmask != NULL) {
		free(s2at30m->acmask);
		s2at30m->acmask = NULL;
	}
	if (s2at30m->fmask != NULL) {
		free(s2at30m->fmask);
		s2at30m->fmask = NULL;
	}

	return 0;
}
