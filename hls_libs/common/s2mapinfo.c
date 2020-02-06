#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "s2mapinfo.h"

int read_s2mapinfo(char *fname_xml, s2mapinfo_t *mapinfo)
{
	FILE *fxml;
	char line[500];
	char *pos, *pos2;
	int len;

	if ((fxml = fopen(fname_xml, "r")) == NULL) {
		fprintf(stderr, "Cannot read %s\n", fname_xml);
		return(-1);
	}
	while (fgets(line, sizeof(line), fxml)) {
		if  (strstr(line, "<HORIZONTAL_CS_NAME>")) {
			 /* Example:
				<HORIZONTAL_CS_NAME>WGS84 / UTM zone 31N</HORIZONTAL_CS_NAME>
			  */
			pos = strstr(line, "zone");
			pos2 = strstr(pos, "<");
			len = pos2-(pos+5);
			strncpy(mapinfo->zonestr, pos+5, len);
			mapinfo->zonestr[len] = '\0';
		}
		else if (strstr(line, "<Size resolution=\"10\">")) {
			/* nrows */
			fgets(line, sizeof(line), fxml);
			pos = strstr(line, ">"); 
			mapinfo->nrow[0] = atoi(pos+1);
			/* ncols */
			fgets(line, sizeof(line), fxml);
			pos = strstr(line, ">"); 
			mapinfo->ncol[0] = atoi(pos+1);

			mapinfo->nrow[1] = mapinfo->nrow[0] / 2;
			mapinfo->ncol[1] = mapinfo->ncol[0] / 2;
			mapinfo->nrow[2] = mapinfo->nrow[0] / 6;
			mapinfo->ncol[2] = mapinfo->ncol[0] / 6;
		}
		else if (strstr(line, "<Geoposition resolution=\"10\">")) {
			/* ulx */
			fgets(line, sizeof(line), fxml);
			pos = strstr(line, ">"); 
			mapinfo->ulx = atof(pos+1);
			/* uly */
			fgets(line, sizeof(line), fxml);
			pos = strstr(line, ">"); 
			mapinfo->uly = atof(pos+1);

			/* Read no more */
			fclose(fxml);
			break;
		}
	}

	return 0;
}
