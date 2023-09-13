#include<stdio.h>
#include<stdlib.h>
#include<math.h>

//����ʬ�Υǡ���������
#define FREQ   (1000)           //�������ȿ�
#define SAMPLE (1000000)        //����ץ�󥰼��ȿ�
#define N      (1000000)        //�ơ��֥���礭��
#define BWIDTH 16

int sin_table[N/4+1];
int max;

void make_table(int n)
{
        int x;
        max = (int)pow(2,BWIDTH-1)-1;

        for (x=0; x<=n/4 ;x++)
          sin_table[x] = (int)(sin(2*M_PI/n*x)*max);
}

int sin_x(int x, int n)
{
        int xx;

        xx = x%(n/4);
        if      (x < n/4*1) return sin_table[xx];
        else if (x < n/4*2) return sin_table[n/4-xx];
        else if (x < n/4*3) return -sin_table[xx];
        else return -sin_table[n/4-xx];
}

main( int argc, char *argv[] )
{
        int     i, d, t=0;
        int     f, n, s;

        f = FREQ;
        s = SAMPLE;
        n = N;

        if (argc>1)
          f = atoi(argv[1]);
        if (argc>2)
          s = atoi(argv[2]);

        n = s;
        make_table(n);

        d = (long)f*n/s;

        for (i=0; i<s ;i++) {
                printf("%d\n",sin_x(t,n));
                t = (t+d)%n;
        }
}