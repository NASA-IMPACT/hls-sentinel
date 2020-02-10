/* Adjust 30m S2 surface reflectance for nadir view and the mean 
 * solar zenith angle at the time of S2 and L8 overpassing the latitude
 * of the tile center for the given day.
 */

#define SENSOR S2

#include "hls_commondef.h"
#include "s2at30m.h"
#include "s2ang.h"
#include "modis_brdf_coeff.h"
#include "rtls.h"
#include "cfactor.h"
#include "mean_solarzen.h"
#include "util.h"

 /* The angle information in the ESA input can be missing for some bands
  * and a substitute will be used if missing. 
  * The angle of which band is used for the bands in a 13-element array.
  */
#define ANGLEBAND  "AngleBand"

int main(int argc, char *argv[])
{
	/* Command line parameters */
	char fname_out[LINELEN];	/* an exact copy of input for update */
	char fname_ang[LINELEN];
	char fname_cfactor[LINELEN];

	s2ang_t s2ang;		/* 30-m angles */
	s2at30m_t s2o;		/* output surface reflectance, after adjustment */
	cfactor_t cfactor;	/* BRDF ancillary; ratio for each band */

	int ib, irow, icol, k; 

	float sz, sa, vz, ra;

	double msz;	/* Mean solar zenith for a location*/
	double rossthick_MSZ, lisparseR_MSZ;	/* kernels at nadir and the mean solar zenith */
	double ratio;
	double tmpref;
	double cenlon, cenlat;

	int ret;

	if (argc != 4) {
		fprintf(stderr, "Usage: %s outsr.hdf ang.hdf cfactor.hdf \n", argv[0]);
		exit(1);
	}

	strcpy(fname_out,     argv[1]);
	strcpy(fname_ang,     argv[2]);
	strcpy(fname_cfactor, argv[3]);

	/* Open output (a copy of input) for update */
	strcpy(s2o.fname, fname_out);
	ret = open_s2at30m(&s2o, DFACC_WRITE);
	if (ret != 0) {
		Error("Error in open_s2at30m");	
		exit(1);
	}

	/* Read angles */
	strcpy(s2ang.fname, fname_ang);
	ret = open_s2ang(&s2ang, DFACC_READ);
	if (ret != 0) {
		Error("Error in open_s2ang");	
		exit(1);
	}
	
	/* Create the brdf ancillary file, of the c factor */
	strcpy(cfactor.fname, fname_cfactor);
	cfactor.nrow = s2o.nrow;
	cfactor.ncol = s2o.ncol;
	ret = open_cfactor(SENTINEL2, &cfactor, DFACC_CREATE);
	if (ret != 0) {
		Error("Error in open_cfactor");
		exit(1);
	}

	/* Solar zenith used in BRDF adjustment. */
	/* Example basename of filename: HLS.S30.T03VXH.2019202.v1.4.hdf */
	char *cp;	
	int yeardoy, year, doy;
	cp = strrchr(s2o.fname, '/');	/* find basename */
	if (cp == NULL)
		cp = s2o.fname;
	else
		cp++;
	/* Skip to yeardoy */
	cp = strchr(cp, '.'); cp++;
	cp = strchr(cp, '.'); cp++;
	cp = strchr(cp, '.'); cp++;
	yeardoy = atoi(cp);
	year = yeardoy / 1000;
	doy = yeardoy - year * 1000;

	msz = mean_solarzen(s2o.zonehem, 
			    s2o.ulx+(s2o.ncol/2.0 * HLS_PIXSZ),
			    s2o.uly-(s2o.nrow/2.0 * HLS_PIXSZ),
			    year,
			    doy);

	write_solarzenith(&s2o, msz);

	/* Processing time */
	char creationtime[50];
	getcurrenttime(creationtime);
	SDsetattr(s2o.sd_id, HLSTIME, DFNT_CHAR8, strlen(creationtime), (VOIDP)creationtime);

	rossthick_MSZ = RossThick(msz, 0, 0);
	lisparseR_MSZ = LiSparseR(msz, 0, 0); 

	/* If the view zenith or azimuth SDS for a band is missing, assign the bandId 
	 * of a band for which the angle SDS are available to indicate which band's 
	 * angle is used.  Do this check for all bands. If the angle SDS are
	 * available, the bandId will be that of the band itself.
	 *
	 */
	uint8 angleband[S2NBAND], sid;
	find_substitute(s2ang.angleavail, angleband);

	int nbaridx = -1;	/* Sequence number of a band in bands with BRDF correction*/
	int specidx;		/* Band index in the MODIS BRDF coefficient array */
	for (ib = 0; ib < S2NBAND; ib++) {
		switch (ib) {
			/*
			case 0:  specidx =  0; break;
			case 1:  specidx =  0; break;
			case 2:  specidx =  1; break;
			case 3:  specidx =  2; break;
			case 4:  specidx = -1; break;
			case 5:  specidx = -1; break;
			case 6:  specidx = -1; break;
			case 7:  specidx =  3; break;
			case 8:  specidx =  3; break;
			case 9:  specidx = -1; break;
			case 10: specidx = -1; break;
			case 11: specidx =  4; break;
			case 12: specidx =  5; break;
			*/
			/* Aug 5, 2019: with the added SDSU coefficients for the red edge bands. 
			 * Landsat processing will skip these bands.
			 * No correction on water vapor or cirrus bands */
			case 0:  specidx =  0; break;
			case 1:  specidx =  0; break;
			case 2:  specidx =  1; break;
			case 3:  specidx =  2; break;
			case 4:  specidx =  3; break;
			case 5:  specidx =  4; break;
			case 6:  specidx =  5; break;
			case 7:  specidx =  6; break;
			case 8:  specidx =  6; break;
			case 9:  specidx = -1; break;
			case 10: specidx = -1; break;
			case 11: specidx =  7; break;
			case 12: specidx =  8; break;
		}
		if (specidx == -1)
			continue;


		nbaridx++;

		sid = angleband[ib];   /* bandId of the substitute band; or the ib itself. */

		/* NBAR */
		for (irow = 0; irow < s2o.nrow; irow++) {
			for (icol = 0; icol < s2o.ncol; icol++) {
				k = irow * s2o.ncol + icol;
				if (s2o.ref[ib][k] == ref_fillval)
					continue;

				/* Bug fix, Sep 6, 2016. Angles for certain bands are not available for some grnaules
				 * due to mistakes in the ESA XML.
				 * Sep 10, 2016: a substitute band may not be able to find.
				 */
				if (s2ang.sz[k] == ANGFILL || s2ang.sa[k] == ANGFILL ||
				    s2ang.vz[sid][k] == ANGFILL || s2ang.va[sid][k] == ANGFILL)
					continue;

				sz = s2ang.sz[k]/100.0;
				vz = s2ang.vz[sid][k]/100.0;
				ra = (s2ang.va[sid][k] - s2ang.sa[k])/100.0;

				ratio = (coeff[specidx][0] + coeff[specidx][1] * rossthick_MSZ + coeff[specidx][2] * lisparseR_MSZ) / 
					(coeff[specidx][0] + coeff[specidx][1] * RossThick(sz, vz, ra) + coeff[specidx][2] * LiSparseR(sz, vz, ra));
				tmpref = s2o.ref[ib][k] * ratio;
				s2o.ref[ib][k] = asInt16(tmpref);

				cfactor.ratio[nbaridx][k] = ratio;
			}
		}
	}

	/* Write angleband as attribute */
	/*
	fprintf(stderr, "angleband:\n");
	for (ib = 0; ib < S2NBAND; ib++) {
		fprintf(stderr, "%d %d\n", s2ang.angleavail[ib], angleband[ib]);
	}
	*/
	if (SDsetattr(s2o.sd_id, ANGLEBAND, DFNT_UINT8, S2NBAND, (VOIDP)angleband) == FAIL) {
		Error("Error in SDsetattr");
		exit(1);
	}

	close_s2ang(&s2ang);
	close_s2at30m(&s2o);
	close_cfactor(&cfactor);

	return 0;
}
