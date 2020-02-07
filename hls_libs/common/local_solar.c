/******************
Hankui Oct 31, 2014
******************/
#define RHO 6371007.181
#include <math.h>
#include <tgmath.h>

// #include "weld.hdf.eos.h" //where PI is defined
#include "local_solar.h"
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
bool sinxy_to_lonlat(double x, double y, double * lon, double * lat){
    double grid_xdim, grid_ydim;
    double PIX_SIZE, north, east;
    double east_179_95; /* this one should be in meters */
    double lon_179_95=179.999999;/* catch in degrees*/

    grid_xdim = ((PI*RHO)/30.0)*2.0;
    grid_ydim = grid_xdim/2.0;


    PIX_SIZE =(RHO * 2. * PI)/grid_xdim;
    /* easting is relative to origin at (-0.5*grid.xdim)*/
    east = x;
    /* northing is relative to origin at (0.5*grid.xdim)*/
    /* grows northing  goes in oposite direction to j growth*/
    north = y;
    *lat = (north/RHO)*(180.0/PI);
    *lon = east/(RHO* cos((*lat)*(PI/180.0)))*(180.0/PI);
    /***************************************************************
    *The catch is here
    *based on max longitudal distance form 0 origin (179.95 degrees)
    ***************************************************************/
    east_179_95 = (lon_179_95*PI/180.0)*(RHO* cos((*lat)*(PI/180.0)));
    if((east_179_95-fabs(east))< pow(10,-12)){
        /* x and y are beyond the sinusoidal blimp*/
        return false;
    }
    return true;
}

//void SolarGeometry(int iYear, int iMonth, int iDay,
//			double dHours, double dMinutes, double dSeconds,
//			double dLatitude, double dLongitude,
//			double *dAzimuth, double *dZenithAngle);


/*
* calculate the GMT time from the local time
*/
void getGMTtimeUS(double dLongitude,int iMonth, int iDay,double dHours, double dMinutes, double dSecondsint,
 int* iMonth_gmt, int *iDay_gmt,double *dHours_gmt, double *dMinutes_gmt, double *dSecondsint_gmt)
{
	int seconds,hours,minutes;
	seconds = (int)((-1.0*dLongitude)*240.0);//240 seconds for each longitude
	hours = seconds/3600;
	seconds = seconds%3600;
	minutes = seconds/60;
	seconds = seconds%60;

	(*dSecondsint_gmt) = dSecondsint+seconds;
	*dMinutes_gmt = dMinutes+minutes;
	*dHours_gmt = dHours+hours;
	*iDay_gmt = iDay;
	*iMonth_gmt = iMonth;
}


/*
* calculate the GMT time from the local time
*/
void getGMTtime(double dLongitude,int doy,double time,
int *iDay_gmt,double *dHours_gmt, double *dMinutes_gmt, double *dSecondsint_gmt)
{
	double gme_time = time-dLongitude/15;//from local to gmt
	*iDay_gmt = doy;

	if (gme_time>24)
	{
		*iDay_gmt = *iDay_gmt+1;
		gme_time = gme_time-24;
		if (*iDay_gmt>365)
		{
			*iDay_gmt = 365;
//			printf("bigger %d day time %f time%f\n",doy,time,gme_time);
		}
	}

	if (gme_time<0)
	{
		*iDay_gmt = *iDay_gmt-1;
		gme_time = gme_time+24;
		if (*iDay_gmt<1)
		{
			*iDay_gmt = 1;
//			printf("smaller %d day time %f time%f day %d\n",doy,time,gme_time,*iDay_gmt);
		}
	}


	*dHours_gmt = (int)gme_time;
	double left_mins = (gme_time-*dHours_gmt)*60;
	*dMinutes_gmt = (int)left_mins;
	*dSecondsint_gmt = (left_mins-*dMinutes_gmt)*60;
}

// leap year bool
bool leapYear (int Year)
{
    if ((( Year % 4 == 0) && (! ( Year % 100 == 0))) || (( Year % 4 == 0) && (! ( Year % 100 == 0))&&( Year % 400 == 0)))
    	return true;
    else
        return false;
}

/*
* get month and day from day of the year
*/
void getDate(int doy,int year,int *month,int *day)
{
	//Set days of each month into an array
	int MonthDays[12] = {31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

	if (leapYear(year))
	{
		int i;
		for (i=1;i<12;i++)
			MonthDays[i]++;
	}
	//Set the name of each month into an array
	//const string MonthName[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
	*month = 0;

	while (MonthDays[*month] < doy)
		*month = (*month + 1) %12;

		//Display month and day
	if (*month>=1)
		*day = doy - MonthDays[*month-1];
	else
		*day = doy;

	(*month)++;
}



//code from http://www.psa.es/sdg/sunpos.htm
void SolarGeometry(int iYear, int iMonth, int iDay,
			double dHours, double dMinutes, double dSeconds,
			double dLatitude, double dLongitude,
			double *dAzimuth, double *dZenithAngle)
{
	// Main variables
	double dElapsedJulianDays;
	double dDecimalHours;
	double dEclipticLongitude;
	double dEclipticObliquity;
	double dRightAscension;
	double dDeclination;

	// Auxiliary variables
	double dY;
	double dX;

	cTime			 udtTime;
	cLocation		 udtLocation;

	udtTime.iYear=iYear;
	udtTime.iMonth=iMonth;
	udtTime.iDay=iDay;
	udtTime.dHours=dHours;
	udtTime.dMinutes=dMinutes;
	udtTime.dSeconds=dSeconds;

	udtLocation.dLatitude=dLatitude;
	udtLocation.dLongitude=dLongitude;

	// Calculate difference in days between the current Julian Day
	// and JD 2451545.0, which is noon 1 January 2000 Universal Time
	{
		double dJulianDate;
		long int liAux1;
		long int liAux2;
		// Calculate time of the day in UT decimal hours
		dDecimalHours = udtTime.dHours + (udtTime.dMinutes
			+ udtTime.dSeconds / 60.0 ) / 60.0;

		// Calculate current Julian Day
		liAux1 =(udtTime.iMonth-14)/12;
		liAux2=(1461*(udtTime.iYear + 4800 + liAux1))/4 + (367*(udtTime.iMonth
			- 2-12*liAux1))/12- (3*((udtTime.iYear + 4900
		+ liAux1)/100))/4+udtTime.iDay-32075;
		dJulianDate=(double)(liAux2)-0.5+dDecimalHours/24.0;

		// Calculate difference between current Julian Day and JD 2451545.0
		dElapsedJulianDays = dJulianDate-2451545.0;
	}

	// Calculate ecliptic coordinates (ecliptic longitude and obliquity of the
	// ecliptic in radians but without limiting the angle to be less than 2*Pi
	// (i.e., the result may be greater than 2*Pi)
	{
		double dMeanLongitude;
		double dMeanAnomaly;
		double dOmega;
		dOmega=2.1429-0.0010394594*dElapsedJulianDays;
		dMeanLongitude = 4.8950630+ 0.017202791698*dElapsedJulianDays; // Radians
		dMeanAnomaly = 6.2400600+ 0.0172019699*dElapsedJulianDays;
		dEclipticLongitude = dMeanLongitude + 0.03341607*sin( dMeanAnomaly )
			+ 0.00034894*sin( 2*dMeanAnomaly )-0.0001134
			-0.0000203*sin(dOmega);
		dEclipticObliquity = 0.4090928 - 6.2140e-9*dElapsedJulianDays
			+0.0000396*cos(dOmega);
	}

	// Calculate celestial coordinates ( right ascension and declination ) in radians
	// but without limiting the angle to be less than 2*Pi (i.e., the result may be
	// greater than 2*Pi)
	{
		double dSin_EclipticLongitude;
		dSin_EclipticLongitude= sin( dEclipticLongitude );
		dY = cos( dEclipticObliquity ) * dSin_EclipticLongitude;
		dX = cos( dEclipticLongitude );
		dRightAscension = atan2( dY,dX );
		if( dRightAscension < 0.0 ) dRightAscension = dRightAscension + DUO_PI;
		dDeclination = asin( sin( dEclipticObliquity )*dSin_EclipticLongitude );
	}

	// Calculate local coordinates ( azimuth and zenith angle ) in degrees
	{
		double dGreenwichMeanSiderealTime;
		double dLocalMeanSiderealTime;
		double dLatitudeInRadians;
		double dHourAngle;
		double dCos_Latitude;
		double dSin_Latitude;
		double dCos_HourAngle;
		double dParallax;

		dGreenwichMeanSiderealTime = 6.6974243242 +
			0.0657098283*dElapsedJulianDays
			+ dDecimalHours;

		dLocalMeanSiderealTime = (dGreenwichMeanSiderealTime*15
			+ udtLocation.dLongitude)*rad;

		dHourAngle = dLocalMeanSiderealTime - dRightAscension;
		dLatitudeInRadians = udtLocation.dLatitude*rad;
		dCos_Latitude = cos( dLatitudeInRadians );
		dSin_Latitude = sin( dLatitudeInRadians );
		dCos_HourAngle= cos( dHourAngle );

		*dZenithAngle = (acos( dCos_Latitude*dCos_HourAngle
			*cos(dDeclination) + sin( dDeclination )*dSin_Latitude));

		dY = -sin( dHourAngle );
		dX = tan( dDeclination )*dCos_Latitude - dSin_Latitude*dCos_HourAngle;

		*dAzimuth = atan2( dY, dX );

		if ( *dAzimuth < 0.0 )
			*dAzimuth = *dAzimuth + DUO_PI;

		*dAzimuth = *dAzimuth/rad;

		// Parallax Correction
		dParallax=(dEarthMeanRadius/dAstronomicalUnit)
			*sin(*dZenithAngle);

		*dZenithAngle=(*dZenithAngle
			+ dParallax)/rad;
	}
}
