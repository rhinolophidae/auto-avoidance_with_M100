#include <stdio.h>

void main ( void)
	{
	char filename[255];
	FILE *fp;
	int i;

	for(i=0; i<15; i++)
		{
		sprintf(filename,"./dat/test%d.dat", i);
		fp=fopen(filename, "w");
		fprintf(fp,"dummy\n");
		fclose(fp);
		}
	}

