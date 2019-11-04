#include "cubic_conv.h"
#include "util.h"

/* Compute cubic convolution weights from the horizontal or vertical distance. */
int cc_weight(double dis[4], double w[4])
{
	int i;
	double alpha = -1;	/* An adjustable parameter */
	char message[MSGLEN];
	
	for (i = 0; i < 4; i++) {
		if (dis[i] < 1)
			w[i] = (alpha+2)*pow(dis[i],3) - (alpha+3)*pow(dis[i],2) + 1;
		else if (dis[i] <= 2)
			w[i] = alpha * (pow(dis[i],3) - 5*pow(dis[i],2) + 8*dis[i] - 4);
		else {
			sprintf(message, "Convolution distance is greater than 2: %lf %lf %lf %lf \n", dis[0], dis[1], dis[2], dis[3]);
			Error(message);
			return (-1);
		}
	}	

	return 0;
}

/* Compute cubic convolution */
double cubic_conv(double val[][4], double xw[4], double yw[4])
{
	int i, j;
	double rowave[4];
	double conv = 0;
	
	for (i = 0; i < 4; i++) {
		rowave[i] = 0;
		for (j = 0; j < 4; j++) 
			rowave[i] += val[i][j] * xw[j];
	}

	for (i = 0; i < 4; i++) 
		conv += rowave[i] * yw[i];

	return conv;
}
