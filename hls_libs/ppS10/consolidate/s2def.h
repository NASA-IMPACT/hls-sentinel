#ifndef S2DEF_H
#define S2DEF_H


/* Still used in resampling to 30m. Really? Dec 20, 2016 */
#define S2NB10M	4 	/* Number of 10m bands */ 
#define S2NB20M	6 	/* Number of 20m bands */ 
#define S2NB60M	3 	/* Number of 60m bands */ 

/* Totally 13 bands, but band10 (cirrus) may not be available in earlier SAFE */
#define S2NBAND 13

/* S2 band names by wavelength*/
/*
static char *S2BANDNAME[] = {"B01", "B02", "B03", "B04", "B05", "B06", "B07", 
			     "B08", "B8A", "B09", "B10", "B11", "B12"};
*/

/* SDS names by wavelength. Use this? Dec 21, 2016 */
static char *S2_SDS_NAME[] = { "B01", "B02", "B03", "B04", "B05", "B06", 
			       "B07", "B08", "B8A", "B09", "B10", "B11", "B12"};

/* Band number in the order of wavelength. 80 is 8a. Needed in BRDF adjustment */
static int S2bandID[] =  {1, 2, 3, 4, 5, 6, 7, 8, 80, 9, 10, 11, 12};

static char *S2_SDS_LONG_NAME[] = { 	"Coastal_Aerosol",
				    	"Blue",
				    	"Green",
				    	"Red", 
					"Red_Edge1",
					"Red_Edge2",
					"Red_Edge3",
					"NIR_Broad",
					"NIR_Narrow",
					"Water_Vapor",
					"Cirrus",
					"SWIR1",
					"SWIR2"};


#endif
