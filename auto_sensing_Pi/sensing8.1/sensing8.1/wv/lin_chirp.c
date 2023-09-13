// lin_chirp.c  down chirp calculation and output waveform
// dc192kwv.dat  ---> 192k sample
// dc1megwv.dat  ---> 1M sample

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define BUFLEN 256
#define T1M 0.000001
#define T192 0.00000521
#define MAX 32767

int main( void)
{
FILE	*fp;
double	fstart, fstop, pduration, pk, fai, t;
char 	buf[BUFLEN];
int		k, DLEN192, DLEN1M;


if( NULL == (fp = fopen("chirp.dat","r")) )
	{	fprintf(stderr, "Cannot open data file\n");	exit(0);	}

fgets(buf, BUFLEN, fp);
fstart = atof(buf);
fprintf(stderr, "Start %e [Hz]\n", fstart);

fgets(buf, BUFLEN, fp);
fstop = atof(buf);
fprintf(stderr, "Stop %e [Hz]\n", fstop);

fgets(buf, BUFLEN, fp);
pduration = atof(buf);
fprintf(stderr, "Duration %e [sec]\n", fstop);
fclose(fp);

pk = (fstart- fstop) / pduration;

DLEN192 = pduration / T192 + 1;
DLEN1M =  pduration / T1M + 1;
fprintf(stderr, "DATALEN %d\t%d\n", DLEN192, DLEN1M);

if( NULL == (fp = fopen("dc192kwv.dat","w")) )
	{	fprintf(stderr, "Cannot open data file\n");	exit(0);	}

for (k=0; k<DLEN192; k++)
	{
	t = T192 * k; 
	fai = 2. * M_PI * ( fstart * t - pk / 2. * t * t);
	fprintf(fp, "%d\n", (int)(sin(fai) * MAX / 2.));
	}
	
	fclose(fp);

if( NULL == (fp = fopen("dc1megwv.dat","w")) )
	{	fprintf(stderr, "Cannot open data file\n");	exit(0);	}

for (k=0; k<DLEN1M; k++)
	{
	t = T1M * k; 
	fai = 2. * M_PI * ( fstart * t - pk / 2. * t * t);
	fprintf(fp, "%d\n", (int)(sin(fai) * MAX /2.));
	}
	
	fclose(fp);



}
