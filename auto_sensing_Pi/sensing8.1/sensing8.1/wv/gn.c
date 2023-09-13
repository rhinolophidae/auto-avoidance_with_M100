#include <stdio.h> 
 
int main (void) 
{
FILE *gp;
int k;
gp = popen("gnuplot -persist","w");

	fprintf(gp, "plot sin(x)\n");
	fflush(gp);
	k=getchar();
	fprintf(gp, "plot cos(x)\n");
	fflush(gp);
//	fprintf(gp, "plot \"tmpdata.dat\"\n");



	k=getchar();
	pclose(gp);	
} 