/******************************************************************************** 
 * Apply the ESA image quality mask that is available in raster format (JP2) 
 * (starting with processing baseline 4.0, roughly from Jan 25, 2022 onward) , 
 * which is much easier to apply than the earlier GML format.  The mask 
 * indicates whether there is a loss of packets for a pixel in a  spectral 
 * band, or there is cross-talk, or saturation.
 *
 * The details of the mask: 
 *	https://sentinels.copernicus.eu/web/sentinel/technical-guides/sentinel-2-msi/data-quality-reports
 *
 *	layer	1: lost ancillary packets, 
 *			2: degraded ancillary packets, 
 *			3: lost MSI packets, 
 *			4: degraded MSI packets, 
 *			5: defective pixels, 
 *			6: no data, 
 *			7: partially corrected cross talk, 
 *			8: saturated pixels. 
 *	"For each spectral band, the mask is defined at the same spatial resolution as that of the spectral band. 
 *	In a first step, layer #7 and 8 (partially corrected and saturated) is not activated" 
 *	(layer 7 is set, contradicting ESA document!)
 *
 *  Decided to mask 3 (lost MSI packets) and 4 (degraded MSI packets) only. So far, 5 and 7 are present in 
 *  B10-B11, because interpolation is used to fill 5 and the cross talk is partially corrected.
 *  Saturated pixels are still useful for the magnitude of the reflectance; keep it.
 *
 * A very late addition, but important.
 * Sept 20, 2022
 * Jan 13, 2023
 * Mar 20, 2023
 ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "hls_projection.h"
#include "s2r.h"
#include "util.h"

int main(int argc, char *argv[])
{
    /* Commandline parameters */
    char fname_s2rin[LINELEN];  /* Open hdf for update */
    char fname_qmB01[LINELEN];   /* B01 quality mask in ENVI format */

    s2r_t s2rin;            
    int irow, icol, nrow, ncol, k;
    int npix;
    int NLAYER = 8; /* Num. of quality layers for each pixel */
    int psi;        /* pixel size index: 0 for 10m, 1 for 20m, 2 for 60m */
    int ib, il;     /* band and layer */

    char creationtime[100];
    char qmname[LINELEN];	/* mask filename for any band */
    char *base, *cpos;
    FILE *qmptr;
    uint8 *mask;
    char junkchar;      /* A junk character for testing file size */
    char message[LINELEN];

	char maskSet = 0;		/* Whether the quality mask is set for a band */
	int8 layer[NLAYER];			/* Which layer of quality mask is set */
    int ret;

    if (argc != 3) {
        fprintf(stderr, "%s s2rin  B01_qual_mask.bin \n", argv[0]);
        exit(1);
    }

    strcpy(fname_s2rin,  argv[1]);
    strcpy(fname_qmB01,  argv[2]);

    /* Open the input for update*/
    strcpy(s2rin.fname, fname_s2rin);
    ret = open_s2r(&s2rin, DFACC_WRITE);
    if (ret != 0) {
        Error("Error in open_s2r");
        exit(1);
    }


    /* Construct the ESA mask filename from B01 filename */
    strcpy(qmname, fname_qmB01); 
    base = strrchr(qmname, '/');
    if (base == NULL)
        base = qmname;
    else
        base++;
	/* base is the basename */
    cpos = strstr(base, "B01");
    if (cpos == NULL) {
        fprintf(stderr, "Filename does not contain characters B01: %s\n", fname_qmB01);
        exit(1);
    }
    
    for (ib = 0; ib < S2NBAND; ib++) {
        strncpy(cpos, S2_SDS_NAME[ib], 3);

        psi = get_pixsz_index(ib);
        nrow = s2rin.nrow[psi];
        ncol = s2rin.ncol[psi];
        npix = nrow*ncol;

        if ((qmptr = fopen(qmname, "r")) == NULL) {
            sprintf(message, "Can't open for read: %s\n", qmname);
            Error(message);
            exit(1);
        }
        /* 8 layers for each pixel but some layers are not used; each layer takes 
		 * 1 byte in the ENVI format; originally JP2 must be using a parsimonious 
		 * representation like a bit field, and a bit is turned into a byte during 
		 * conversion by GDAL.
         */ 
        if ((mask = (uint8*)calloc(npix*NLAYER, sizeof(uint8))) == NULL) {
            Error("Can't allocate memory");
            exit(1);
        }
        if (fread(mask, 1, npix*NLAYER, qmptr) != npix*NLAYER ||
            fread(&junkchar, 1, 1, qmptr) == 1) {

            /* File size should be exact; the extra read attempt is useful guard against
			 * error  */
            sprintf(message, "File size is wrong: %s\n", qmname);
            Error(message);
            exit(1);
        } 
        
		maskSet = 0;
		for (il = 0; il < NLAYER; il++) 
			layer[il] = -1;
        for (irow = 0; irow < nrow; irow++) {
            for (icol = 0; icol < ncol; icol++) {
                k = irow * ncol + icol;
                for (il = 0; il < NLAYER; il++) {
                    if (mask[il*npix+k] == 1) {
						if (il == 2 || il == 3)		/* lost or degraded MSI packets; most severe problem */
							s2rin.ref[ib][k] = HLS_REFL_FILLVAL;

						maskSet = 1;
						layer[il] = il;		/* Assuming only one layer is set for a given band */

					}

                }
            }
        }

		if (maskSet) { 
			fprintf(stderr, "maskname = %s\n", qmname); 
            for (il = 0; il < NLAYER; il++) {
				if (layer[il] != -1)
					fprintf(stderr, "Band %s quality mask is set for layer %d\n", S2_SDS_NAME[ib], layer[il]+1);
			}
			fprintf(stderr, "\n");
		}

        free(mask);
    }

    /* Processing time */
    getcurrenttime(creationtime);
    SDsetattr(s2rin.sd_id, HLSTIME, DFNT_CHAR8, strlen(creationtime), (VOIDP)creationtime);

    if (close_s2r(&s2rin) != 0) {
        Error("Error in closing output");
        exit(1);
    }

    return 0;
}
