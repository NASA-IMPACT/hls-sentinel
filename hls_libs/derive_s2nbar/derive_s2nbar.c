/* Adjust the 30m S2 surface reflectance to nadir view and the mean solar 
 * zenith angle for the day at the respective time of S2 and L8 overpassing the 
 * latitude of the tile center. For high latitude where the tile center is 
 * above the hightest latitude that a sensor's nadir can get, use the mean of
 * the observed solar zenith in each granule.
 *
 * Revised on May 15, 2020.
 * Sep 7, 2020: All bands use the same view zenith and azimuth.
 */

/*
#################### Credit -- Text from David Roy's group:
#  ! The modeled solar zenith for HLS NBAR (v1.5) is defined as a function of latitude and day of
#  ! the year (Li et al. 2019), and is calculated through a sensor overpass time model
#  ! (Eq. 4 in Li et al. 2019).  The purpose of this definition is to resembling the observed
#  ! solar zeniths (Zhang et al. 2016) as the c-factor approach (Roy et al. 2016) used in HLS
#  ! NBAR derivation is for viewing zenith correction and does not allow solar zenith extrapolation.
#  ! Note that the local overpass time at the Equator and the satellite inclination angle used in
#  ! the sensor overpass time model (Eq. 4 in Li et al. 2019) take the average of the values for
#  ! Landsat-8 and Sentinel-2.
#
#  ! Li, Z., Zhang, H.K., Roy, D.P., 2019. Investigation of Sentinel-2 bidirectional reflectance
#  ! hot-spot sensing conditions. IEEE Transactions on Geoscience and Remote Sensing, 57(6), 3591-3598.
#  !
#  ! Roy, D.P., Zhang, H. K., Ju, J., Gomez-Dans, J. L., Lewis, P.E., Schaaf C.B., Sun, Q., Li, J.,
#  ! Huang, H., Kovalskyy, V., 2016. A general method to normalize Landsat reflectance data to nadir
#  ! BRDF adjusted reflectance. Remote Sensing of Environment, 176, 255-271.
#  !
#  ! Zhang, H. K., Roy, D.P., Kovalskyy, V., 2016. Optimal solar geometry definition for global
#  ! long term Landsat time series bi-directional reflectance normalization. IEEE Transactions
#  ! on Geoscience and Remote Sensing, 54(3), 1410-1418.
################################################################################
*/

#include "hls_commondef.h"
#include "s2at30m.h"
#include "s2ang.h"
#include "modis_brdf_coeff.h"
#include "rtls.h"
#include "cfactor.h"
#include "mean_solarzen.h"
#include "util.h"

#define NBARSZ  "NBAR_SOLAR_ZENITH"
int write_nbar_solarzenith(s2at30m_t *s2o, double nbarsz);

/* The mean solar and view zenith/azimuth angles.
 * Not essential quantities, but a possible use of the mean sun angle is: For tiles with 
 * its centers above the orbit nadir, the mean solar zenith may be used in NBAR because
 * the desired normalized solar zenith based on Landsat and Sentinel-2 overpass time 
 * can't be derived.
 */
int write_mean_angle(s2at30m_t *s2o, double msz, double msa, double mvz, double mva);

int main(int argc, char *argv[])
{
	/* Command line parameters */
	char fname_out[LINELEN];	/* an exact copy of input for update */
	char fname_ang[LINELEN];
	char fname_cfactor[LINELEN];	/* C-factor file, not archived */

	s2ang_t s2ang;		/* 30-m angles */
	s2at30m_t s2o;		/* output surface reflectance, after adjustment */
	cfactor_t cfactor;	/* BRDF ancillary; ratio for each band */

	int ib, irow, icol, k; 

	float sz, sa, vz, va, ra;

	double nbarsz;	/* Mean solar zenith for a location*/
	double rossthick_nbarsz, lisparseR_nbarsz;	/* kernels at nadir and the mean solar zenith */
	double ratio;
	double tmpref;
	int utmzone;
	double cenx, ceny, cenlon, cenlat;

	/* The mean angles in band 0 as metadata */
	double msz, msa, mvz, mva;
	int n;
	msz = msa = mvz = mva = 0;
	n = 0;


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

	/* Calculate the mean solar zenith and azimuth in case that the input granule is
	 * a consolidated one from twin granules, the mean values will be different from
	 * the L1C values of the twin granules. For the same reason, calculate the mean 
	 * view zenith and azimuth.
	 * These mean values angles are written out as metadata.
	 *
	 * May 15, 2020: For very high latitude, the calculated mean solar zenith
	 * will also be used for NBAR since an "ideal" NBAR solar zenith can't be derived.
	 */
	ib = 0; 	/* coastal/aerosol band */
	for (irow = 0; irow < s2o.nrow; irow++) {
		for (icol = 0; icol < s2o.ncol; icol++) {
			k = irow * s2o.ncol + icol;
			if (s2o.ref[ib][k] == ref_fillval)
				continue;

			if (s2ang.ang[0][k] == ANGFILL || s2ang.ang[1][k] == ANGFILL ||
			    s2ang.ang[2][k] == ANGFILL || s2ang.ang[3][k] == ANGFILL)
				continue;

			sz = s2ang.ang[0][k]/100.0;
			sa = s2ang.ang[1][k]/100.0;
			vz = s2ang.ang[2][k]/100.0;
			va = s2ang.ang[3][k]/100.0;

			n++;
                        msz = msz + (sz-msz)/n;
                        msa = msa + (sa-msa)/n;
                        mvz = mvz + (vz-mvz)/n;
                        mva = mva + (va-mva)/n;
		}	
	}

	/*** Derive solar zenith used in BRDF adjustment. 
	 *
	 * Sentinel-2 nadir does not go higher than 81.38 deg and Landsat does not go 
	 * higher than 81.8.
	 * Compute the tile center latitude rather than reading from a file. 
	 */
	utmzone = atoi(s2o.zonehem);
	cenx = s2o.ulx + (s2o.ncol/2.0 * HLS_PIXSZ),
	ceny = s2o.uly - (s2o.nrow/2.0 * HLS_PIXSZ);
	if (strstr(s2o.zonehem, "S") && ceny > 0)  /* Sentinel 2 */ 
		ceny -= 1E7;		/* accommodate GCTP */
	utm2lonlat(utmzone, cenx, ceny, &cenlon, &cenlat);

	fprintf(stderr, "cenlat = %lf\n", cenlat);
	if (cenlat > 81.3) 
		nbarsz = msz;
	else {
		/* Example basename of filename: HLS.S30.T03VXH.2019202.v1.4.hdf 
	 	 * 				 HLS.S30.T03VXH.2019202TXXXXXX.v1.4.hdf 
		 */
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

		nbarsz = mean_solarzen(s2o.zonehem, cenx, ceny, year, doy);
	}

	/* Processing time */
	char creationtime[50];
	getcurrenttime(creationtime);
	SDsetattr(s2o.sd_id, HLSTIME, DFNT_CHAR8, strlen(creationtime), (VOIDP)creationtime);

	/* Kernel values for NBAR solar zenith and nadir view */
	rossthick_nbarsz = RossThick(nbarsz, 0, 0);
	lisparseR_nbarsz = LiSparseR(nbarsz, 0, 0); 


	int nbaridx = -1;	/* Sequence number of a band in bands with BRDF correction*/
	int specidx;		/* Band index in the MODIS BRDF coefficient array */
	for (ib = 0; ib < S2NBAND; ib++) {
		switch (ib) {
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

		/* NBAR */
		for (irow = 0; irow < s2o.nrow; irow++) {
			for (icol = 0; icol < s2o.ncol; icol++) {
				k = irow * s2o.ncol + icol;
				if (s2o.ref[ib][k] == ref_fillval)
					continue;

				/* Bug fix, Sep 6, 2016. Angles for certain bands are not available for some grnaules
				 * due to mistakes in the ESA XML.
				 * Sep 10, 2016: a substitute band may not be able to find.
				 *
				 *  ang[0] is solar zenith, 1 is solar azimuth, 2 is view zenith, 3 is view azimuth
				 */

				if (s2ang.ang[0][k] == ANGFILL || s2ang.ang[1][k] == ANGFILL ||
				    s2ang.ang[2][k] == ANGFILL || s2ang.ang[3][k] == ANGFILL)
					continue;

				sz = s2ang.ang[0][k]/100.0;
				sa = s2ang.ang[1][k]/100.0;
				vz = s2ang.ang[2][k]/100.0;
				va = s2ang.ang[3][k]/100.0;
				ra = va - sa;

				ratio = (coeff[specidx][0] + coeff[specidx][1] * rossthick_nbarsz + coeff[specidx][2] * lisparseR_nbarsz) / 
					(coeff[specidx][0] + coeff[specidx][1] * RossThick(sz, vz, ra) + coeff[specidx][2] * LiSparseR(sz, vz, ra));
				tmpref = s2o.ref[ib][k] * ratio;
				s2o.ref[ib][k] = asInt16(tmpref);

				cfactor.ratio[nbaridx][k] = ratio;
			}
		}
	}

	write_nbar_solarzenith(&s2o, nbarsz);
	write_mean_angle(&s2o, msz, msa, mvz, mva);

	close_s2ang(&s2ang);
	close_s2at30m(&s2o);
	close_cfactor(&cfactor);

	return 0;
}

int write_nbar_solarzenith(s2at30m_t *s2o, double nbarsz)
{
	int ret;
	ret = SDsetattr(s2o->sd_id, NBARSZ, DFNT_FLOAT64, 1, (VOIDP)&nbarsz);
	if (ret != 0) {
                Error("Error in SDsetattr");
                exit(-1);
        }

        return(0);
}

int write_mean_angle(s2at30m_t *s2o, double msz, double msa, double mvz, double mva)
{
	int ret; 
	/* MSZ */
	ret = SDsetattr(s2o->sd_id, MSZ, DFNT_FLOAT64, 1, (VOIDP)&msz);
	if (ret != 0) {
		Error("Error in SDsetattr");
		exit(-1);
	}

	/* MSA */
	ret = SDsetattr(s2o->sd_id, MSA, DFNT_FLOAT64, 1, (VOIDP)&msa);
	if (ret != 0) {
		Error("Error in SDsetattr");
		exit(-1);
	}

	/* MVZ */
	ret = SDsetattr(s2o->sd_id, MVZ, DFNT_FLOAT64, 1, (VOIDP)&mvz);
	if (ret != 0) {
		Error("Error in SDsetattr");
		exit(-1);
	}
	/* MVA */
	ret = SDsetattr(s2o->sd_id, MVA, DFNT_FLOAT64, 1, (VOIDP)&mva);
	if (ret != 0) {
		Error("Error in SDsetattr");
		exit(-1);
	}

	return(0);
}
