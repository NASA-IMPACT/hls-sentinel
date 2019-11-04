#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "read_arop_output.h"


int read_aropstdout(char *fname_aropstdout, 
		    double *warpulx,
		    double *warpuly,
		    double *newwarpulx,
		    double *newwarpuly,
		    int *ncp,
		    double *rmse,
		    double coeff[2][3])
{
	FILE *farop;
	char *pos;
	char line[LINELEN];
	char message[MSGLEN];
	char foundNewUL;	/* Sometimes the new UL is not derived due to lack of enough control points. */
	char foundPoly;

	/* Warning in AROP */
	char *testfail = "matching test failed, use with CAUTION!!!"; 

	if ((farop = fopen(fname_aropstdout, "r")) == NULL) {
		sprintf(message, "Cannot open %s", fname_aropstdout);
		Error(message);
		return(1);
	}

	/*
	computing new upper left corner for WARP images based on control points
		Num_CP_Candidates: 38	Num_CP_Selected: 24
		base image UL:  (  199980.0, -3499980.0)
		warp image UL:  (  191985.0, -3557085.0)
		warp images may be adjusted with new UL: (  191991.6, -3557065.0)
	trying 1st order polynomial transformation
		Total_Control_Points=38,  Used_Control_Points=18, RMSE= 0.168
		x' =   266.261 +  1.000197 * x + -0.000163 * y
		y' = -1902.747 + -0.000025 * x +  0.999984 * y
	*/
	foundNewUL = 0;
	foundPoly = 0;
	while (fgets(line, sizeof(line), farop)) {
		if (strstr(line, "warp image UL:")) {
			pos = strrchr(line, '(');
			*warpulx = atof(pos+1);
			pos = strrchr(line, ',');
			*warpuly = atof(pos+1);
		}

		if (strstr(line, "warp images may be adjusted with new UL:")) {
			pos = strrchr(line, '(');
			*newwarpulx = atof(pos+1);
			pos = strrchr(line, ',');
			*newwarpuly = atof(pos+1);

			foundNewUL = 1;
		}

		if ((pos = strstr(line, "Used_Control_Points")) != NULL) {
			pos = strstr(pos, "=");
			*ncp = atoi(pos+1);

			pos = strstr(pos+1, "=");
			*rmse = atof(pos+1) * 10; /* From 10m pixels to meters */
		}

		if (strstr(line, "x'") && strstr(line, "=") && strstr(line, "*")) {
			pos = strstr(line, "=");
			coeff[0][0] = atof(pos+1);

			pos = strstr(pos+1, "+");
			coeff[0][1] = atof(pos+1);

			pos = strstr(pos+1, "+");
			coeff[0][2] = atof(pos+1);

			foundPoly = 1;
		}

		if (strstr(line, "y'") && strstr(line, "=") && strstr(line, "*")) {
			pos = strstr(line, "=");
			coeff[1][0] = atof(pos+1);

			pos = strstr(pos+1, "+");
			coeff[1][1] = atof(pos+1);

			pos = strstr(pos+1, "+");
			coeff[1][2] = atof(pos+1);
		}

		if (strstr(line, testfail)) {
			/*
			sprintf(message, "%s  %s", testfail, fname_aropstdout);
			Error(message);
			*/
			return(1);
		}
	}

	fclose(farop);

	if (foundNewUL && foundPoly)
		return(0);
	else
		return(1);
}
