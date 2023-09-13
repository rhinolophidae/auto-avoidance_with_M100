#include<stdio.h>
#include<math.h>
#include<stdlib.h>

#define SAMPLE 100000
#define MAX      32767
#define POINT  100000

int main( int argc, char *argv[] )
{
        double d,f;
        double out;
        int t;

        f = atoi(argv[1]);
        d = 2*M_PI*f/SAMPLE;

        for (t=0; t<POINT ;t++) {
          out = sin(t*d);
          printf("%d\n",(int)(out*MAX/2));
        }
}
