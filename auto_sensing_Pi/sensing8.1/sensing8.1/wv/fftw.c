#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include "fftw3.h"

#define SIZE 10000

#define BOXNUM  10000
double txwfm[BOXNUM];

void fft_tx(void )    // perform FFT & conjugate of tx wavefrom; result contaned to struct CMPLX txspc[BOXNUM]
	{
	int i;
	int N;
	double re,im;
	fftw_complex *in, *out;
	fftw_plan p;

//	FILE *fp;
	
	N = BOXNUM; // FFT length

	in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex)*N);
   out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex)*N);

   for(i=0; i<N; i++) 	{ 
		in[i][0] = txwfm[i];    		// �¿���
      	in[i][1] = 0.0;     			// ������		
		}

   p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE); 
	fftw_execute(p);
	
	for (i=0; i<BOXNUM; i++)	{
		re = out[i][0];	
		im = -out[i][1];			// negative sign to obtain conjugate
		}

// delete (//) to output tx spectrum data 	
//	if (NULL == ( fp=fopen("chk_txsp.dat","w")))
//		{	fprintf(stderr, "fileopen error!");	exit(3);	}
//	for (i=0; i< BOXNUM; i++)		
//			fprintf(fp, "%f\n", sqrt(pow(base[i].re, 2) + pow(base[i].im, 2)));
//	fclose(fp);

	fftw_destroy_plan(p);
 	fftw_free(in);
  	fftw_free(out);

   return;
	}



int main( int argc, char *argv[])

{
        FILE *fp;
        int i;
        int N;
 //       double *Y; 
	double Y[SIZE];
        double re,im,mag,ang;
        fftw_complex *in, *out;
        fftw_plan p;

        // �ǡ����ɹ����ѤΥХåե������
//        if (!(Y=(double *)malloc(SIZE))) exit(-1);

        // �ǡ����ɹ��ߡʥǡ����Ŀ���Ĵ�٤��
        for (N=0; scanf("%lf",&Y[N])!=EOF && N<SIZE ;N++) ;
	if (N<=3)
		printf("no data\n");
	fprintf(stderr,"N=%d\n", N);

        //N�Ĥ�FFTW�Ѥ�ʣ�ǿ��������ݤ���
        in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex)*N);
        out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex)*N);
        
        // ʣ�ǿ���������ϥǡ����򥻥å�
        for(i=0; i<N; i++) {            // t=0��1sec�Υǡ���
                in[i][0]=Y[i];          // �¿���
                in[i][1]=0.0;           // ������
        }

        // �������Υա��ꥨ�Ѵ���¹�
        p = fftw_plan_dft_1d(N,in,out,FFTW_FORWARD,FFTW_ESTIMATE);
        fftw_execute(p); 
        

        // ��̤���Ϥ���
        for(i=0; i<N; i++) {            // f=0��N�ʥ���ץ�󥰼��ȿ��ˤΥǡ���
                re = out[i][0];         // ʣ�ǿ��μ¿���
                im = out[i][1];         // ʣ�ǿ��ε�����
                mag = sqrt(re*re + im*im);      // �礭����׻�
                ang = atan2(im,re);             // ����Ѥ�׻�
                printf("%d \t %.5lf \t %.5lf \t %.5lf \t %.5lf \n", i,re,im,mag,ang);
        }

        // ������ʻ��Ѥ������������Ѵ���
        fftw_destroy_plan(p);
        fftw_free(in);
        fftw_free(out);

//        free(Y);
}
