#include "cfactor.h"
#include "s2r.h"
#include "lsat.h"
#include "hls_commondef.h"
#include "error.h"

int open_cfactor(int sensor_type, cfactor_t *cfactor, intn access_mode)
{
	int  ib;
	char all_sds_name[NBAND_CFACTOR][200];
	char message[MSGLEN];

	char sds_name[500];     
	int32 sds_index;
	int32 sd_id, sds_id;
	int32 nattr, attr_index;
	//char attr_name[200];
	int32 dimsizes[2];
	int32 rank, data_type, n_attrs;
	int32 start[2], edge[2];

	if (sensor_type == SENTINEL2) {
		cfactor->nband = NBAND_CFACTOR_S2;
		strcpy(all_sds_name[0], S2_SDS_NAME[0]);
		strcpy(all_sds_name[1], S2_SDS_NAME[1]);
		strcpy(all_sds_name[2], S2_SDS_NAME[2]);
		strcpy(all_sds_name[3], S2_SDS_NAME[3]);
		strcpy(all_sds_name[4], S2_SDS_NAME[4]);
		strcpy(all_sds_name[5], S2_SDS_NAME[5]);
		strcpy(all_sds_name[6], S2_SDS_NAME[6]);
		strcpy(all_sds_name[7], S2_SDS_NAME[7]);
		strcpy(all_sds_name[8], S2_SDS_NAME[8]);
		strcpy(all_sds_name[9], S2_SDS_NAME[11]);
		strcpy(all_sds_name[10],S2_SDS_NAME[12]);
	}
	else if (sensor_type == LANDSAT8) {
		cfactor->nband = NBAND_CFACTOR_L8;
		for (ib = 0; ib < cfactor->nband; ib++) 
			strcpy(all_sds_name[ib], L8_REF_SDS_NAME[1][ib]);	/* cirrus in the last of the reflectance band; dropped */
	}
	else {
		fprintf(stderr, "Sensor type is not considered: %d\n", sensor_type);
		exit(1);
	}

	cfactor->sd_id = FAIL;
	for (ib = 0; ib < cfactor->nband; ib++) {
		cfactor->ratio[ib] = NULL;
		cfactor->sds_id_ratio[ib] = FAIL;
	}
	cfactor->access_mode = access_mode;

	if (access_mode == DFACC_CREATE) {
		char *dimnames[] =  {"YDim_Grid", "XDim_Grid"};
		int32 comp_type;   /*Compression flag*/
		comp_info c_info;  /*Compression structure*/
		comp_type = COMP_CODE_DEFLATE;
		c_info.deflate.level = 2;     /*Level 9 would be too slow */
		rank = 2;
		int irow, icol;

		if ((cfactor->sd_id = SDstart(cfactor->fname, access_mode)) == FAIL) {
			sprintf(message, "Cannot create %s", cfactor->fname);
			Error(message);
			return(ERR_CREATE);
		}

		dimsizes[0] = cfactor->nrow;
		dimsizes[1] = cfactor->ncol;
		for (ib = 0; ib < cfactor->nband; ib++) {
			strcpy(sds_name, all_sds_name[ib]);
			if ((cfactor->sds_id_ratio[ib] = SDcreate(cfactor->sd_id, sds_name, DFNT_FLOAT32, rank, dimsizes)) == FAIL) {
				sprintf(message, "Cannot create SDS %s", sds_name);
				Error(message);
				return(ERR_CREATE);
			}    
			/* Added later, but cause seg fault. SDsetattr can only be called once?   Apr 16, 2016.
			SDsetattr(cfactor->sds_id_ratio[ib], "long_name",  
					DFNT_CHAR8, strlen(all_sds_name[ib]), (VOIDP)all_sds_name[ib]);
			*/
			PutSDSDimInfo(cfactor->sds_id_ratio[ib], dimnames[0], 0);
			PutSDSDimInfo(cfactor->sds_id_ratio[ib], dimnames[1], 1);
			SDsetcompress(cfactor->sds_id_ratio[ib], comp_type, &c_info);	
			SDsetattr(cfactor->sds_id_ratio[ib], "_FillValue", DFNT_FLOAT32, 1, (VOIDP)&cfactor_fillval);

			if ((cfactor->ratio[ib] = (float32*)calloc(cfactor->nrow * cfactor->ncol, sizeof(float32))) == NULL) {
				Error("Cannot allocate memory for cfactor->qa\n");
				return(1);
			}
			for (irow = 0; irow < cfactor->nrow; irow++) {
				for (icol = 0; icol < cfactor->ncol; icol++) 
					cfactor->ratio[ib][irow * cfactor->ncol + icol] = cfactor_fillval;
			}
		}

	}
	else {
		fprintf(stderr, "Access mode not implemented for cfactor: %d\n", access_mode);
		exit(1);
	} 

	return(0);
}


int close_cfactor(cfactor_t *cfactor) 
{
	int ib;
	
	if (cfactor->access_mode == DFACC_CREATE && cfactor->sd_id != FAIL) {
		int32 start[2], edge[2];

		for (ib = 0; ib < cfactor->nband; ib++) {
			if (cfactor->sds_id_ratio[ib] == FAIL)
				continue;

			start[0] = 0; edge[0] = cfactor->nrow;
			start[1] = 0; edge[1] = cfactor->ncol;
			if (SDwritedata(cfactor->sds_id_ratio[ib], start, NULL, edge, cfactor->ratio[ib]) == FAIL) {
				Error("Error in SDwritedata");
				return(ERR_CREATE);
			}
			SDendaccess(cfactor->sds_id_ratio[ib]);
		}

		SDend(cfactor->sd_id);
		cfactor->sd_id = FAIL;
	}


	for (ib = 0; ib < cfactor->nband; ib++) {
		if (cfactor->ratio[ib] != NULL) {
			free(cfactor->ratio[ib]);
			cfactor->ratio[ib] = NULL;
		}
	}

	return(0);
}
