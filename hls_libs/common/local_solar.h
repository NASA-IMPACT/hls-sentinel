/******************
Hankui Oct 31, 2014
******************/

#include <stdbool.h>

#ifndef WELD_LOCAL_SOLAR
#define WELD_LOCAL_SOLAR

typedef struct _cTime
{
	int iYear;
	int iMonth;
	int iDay;
	double dHours;
	double dMinutes;
	double dSeconds;
} cTime;

typedef struct _cLocation
{
	double dLongitude;
	double dLatitude;
} cLocation;

#define M_PI			3.14159265358979323846
#define M_PI_2			1.57079632679489661923
#define M_PI_4			0.78539816339744830962
#define M_PI			3.14159265358979323846
#define M_PI_2			1.57079632679489661923
#define M_PI_4			0.78539816339744830962
#define DUO_PI			(M_PI*2.0)

#ifndef PI
#define PI (M_PI)
#endif

#define rad	 (M_PI/180)
#define dEarthMeanRadius	 6371.007181	// In km 6371007.181 in m
#define dAstronomicalUnit	149597890	// In km

/************************************************************************
 *  Name: sinxy_to_lonlat()
 *    Purpose: Calculate the geographic coordinates of the singrod pixel and tell
 *            if it within the sinusoidal blimp
 *
 *  Args:
 *          @x                 - x coordinate of point in question in sin proj
 *          @y                - y coordinate of point in question in sin proj
 *          @lon                 - longitude coordinate of point in question
 *          @lat                - latitude coordinate of point in question
 * Calls: main()
 ***********************************************************************/
bool sinxy_to_lonlat(double x, double y, double * lon, double * lat);

void SolarGeometry(int iYear, int iMonth, int iDay,
			double dHours, double dMinutes, double dSeconds,
			double dLatitude, double dLongitude,
			double *dAzimuth, double *dZenithAngle);


/*
* calculate the GMT time from the local time
*/
void getGMTtimeUS(double dLongitude,int iMonth, int iDay,double dHours, double dMinutes, double dSecondsint,
 int* iMonth_gmt, int *iDay_gmt,double *dHours_gmt, double *dMinutes_gmt, double *dSecondsint_gmt);

/*
* calculate the GMT time from the local time
*/
void getGMTtime(double dLongitude,int doy,double time,
int *iDay_gmt,double *dHours_gmt, double *dMinutes_gmt, double *dSecondsint_gmt);

/*
* get month and day from day of the year
*/
void getDate(int doy,int year,int *month,int *day);


#endif
