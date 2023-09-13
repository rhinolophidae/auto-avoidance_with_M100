// singen2.c

#include<stdio.h>
#include<math.h>
#include<stdlib.h>

#define SAMPLE 192000
#define MAX      32767
#define POINT  100000

int main( int argc, char *argv[] )
{
        double d,f;
        double out;
        int t, pts;

        f = (double)atoi(argv[1]);
        d = 2.*M_PI*f/SAMPLE;
		  pts = atoi(argv[2]);

        for (t=0; t<pts ;t++) {
          out = sin((double)t * d);
          printf("%d\n",(int)(out*MAX/2.));
        }
}
