//pulse compression method pcm03.c
// 		2018.7.30  	ver. 1.1  Hilbert Transform   |  Digital Low Pass Filtering
//		2018.8.3		ver. 2.0	with GUI				
//		2018.8.20		ver. 2.1
//	  	2018.10.24		change pulse from lin to exp  ああああ
//	  	2019.07.02		2ch化のための関数化
//	  	2019.07.08		2ch化のtest用　左右の相関波形が取れているかをを見るプログラム
//		2019.07.08		無駄なところをすべて消去。説明追加と整理
//		2019.07.10		ver07 結果のtxtを別ファイルに保存 
//		2019.07.11		ver07.1 ピーク値のval,すなわち音圧も使ってペアリングする
//		2019.07.20		ver07.2 FAILCOUNT=3,ファイルの書き換え処理の見直し、無駄な処理の削除,startcarbatではsingleのときに首を降るように変更.
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <usb.h>
#include <time.h>
#include <math.h>
#include <wiringPi.h>

#include "mailbox.h"
#include "gpu_fft.h"
#include "local_avdeg_cal.h"
#include "ftd2xx.h"
// 10m/340m = 29.4msec
// 1MHz sample
// 29.4x(0.001)x1000000 = 29.4kpts
// 

#define BOXNUM  32768   //  data points for calculating array
#define FByte 240000 	//1回のFT_Read命令によって取得するデータ数
#define DataNum FByte*4/3//取得するデータは320000ワード
#define TRIGTSD	960
#define FS 1.e6        //  sampling frequency
#define MASKLEN 800
#define BUFLEN 256

#define TIMEOUT (5*1000)
#define EP6	0x86

#define direct_pulse_time 500 //直龍波us 実際は 50000/340 = 147 us
#define failmax  3 //1default は10 
#define peaksmax  50 //
#define thd_limit  0.2 //相関波形のMAX値に対する割合.閾値になる defalt は0.2

#define GPIO25 25
#define GPIO8  8
#define ADC_BYTE_NUM FByte/3
unsigned char buf1[FByte];//USBデータ取得用バッファとして使う
unsigned char buf2[FByte];//USBデータ取得用バッファとして使う
unsigned char buf3[FByte];//USBデータ取得用バッファとして使う
unsigned char buf4[FByte];//USBデータ取得用バッファとして使う
unsigned char buf[FByte*4];

// FTDI IC変数設定
FT_HANDLE ftHandle;
FT_STATUS ftstatus;
DWORD EventDWord;
DWORD RxBytes1;
DWORD TxBytes;
DWORD BytesReceived;
WORD adc_1[FByte];
WORD adc_2[FByte];

struct CMPLX {	double re, im;	};

struct usb_bus *sbus;
struct usb_device *sdev;
usb_dev_handle *sdh;

void* thread1(void* pParam);  // HiFiBerry DAC wav file output
void* thread1_short(void* pParam);
void* thread2(void* pParam);  // FX2 USB  ADC data input

double txwfm[BOXNUM], rxwfm[BOXNUM], crwv[BOXNUM];		// tx & rx waveform with 1MHz sample
struct CMPLX txspc[BOXNUM], rxspc[BOXNUM], mxspc[BOXNUM], crwfm[BOXNUM], 
				mxspc2[BOXNUM], crwfm2[BOXNUM];

int		nrep, flg_twv, flg_tsp, flg_rwv, flg_rsp, flg_crsp, flg_crwv, flg_peak;
int leftpeaks[BOXNUM],rightpeaks[BOXNUM];
double leftpeaks_val[BOXNUM],rightpeaks_val[BOXNUM];
int lflag=0, rflag=0; // peaks number
int trialnum,multipulseflag,neckflag,duration_short_ratio;


void tx_wfm_load(int short_ratio )     // load tx waveform file:dc1megwv.dat output from lin_chirp 
	{
	FILE	*fp;
	int	i;
	char  cbuf[255];
	
	for (i=0; i< BOXNUM; i++)		txwfm[i] = 0.;

	//if (NULL == ( fp=fopen("dc1megwv.dat","r")))
	//	{	fprintf(stderr, "fileopen error!");	exit(2);	}
	if(short_ratio ==1)
		{fp=fopen("dc1megwv.dat","r");}
	else
		{fp=fopen("dc1megwv_short.dat","r");}

	i = 0;
	while ( NULL != fgets(cbuf,255, fp) )
		{
		txwfm[i] = atof(cbuf);		i++;
		}
	fclose(fp);

	if (flg_twv == 1)
		{
		if (NULL == ( fp=fopen("result_sensing/chk_txwv.dat","w")))
			{	fprintf(stderr, "fileopen error!");	exit(0);	}
		for (i=0; i< BOXNUM; i++)		
				fprintf(fp, "%f\n", txwfm[i]);
		fclose(fp);
		}

	return;
	}


void fft_tx(void )    // perform FFT & conjugate of tx wavefrom; result contaned to struct CMPLX txspc[BOXNUM]
	{
	int		i, log2_N, N, ret, mb;
 	struct GPU_FFT_COMPLEX *base;
 	struct GPU_FFT *fft;
	FILE *fp;
	
	log2_N = 15; // for 32768pts
	N = 1<<log2_N; // FFT length
	mb = mbox_open();

        ret = gpu_fft_prepare(mb, log2_N, GPU_FFT_FWD, 1, &fft); 
	base = fft->in; // input buffer
	
	for (i=0; i<BOXNUM; i++)	{
		base[i].re = txwfm[i];		base[i].im  = 0.;		}

	gpu_fft_execute(fft);	
	base = fft->out;

	for (i=0; i<BOXNUM; i++)	{
		txspc[i].re = (double)base[i].re;	
		txspc[i].im = (double)(-base[i].im);	// negative sign to obtain conjugate
		}

	if (flg_tsp == 1)
		{
		if (NULL == ( fp=fopen("result_sensing/chk_txsp.dat","w")))
			{	fprintf(stderr, "fileopen error!");	exit(3);	}
		for (i=0; i< BOXNUM; i++)		
				fprintf(fp, "%f\n", sqrt(pow(base[i].re, 2) + pow(base[i].im, 2)));
		fclose(fp);
		}

	gpu_fft_release(fft); // Videocore memory lost if not freed !
	mbox_close(mb);
   return;
	}

void make_rx(char* receiver_side)//生波形データを変数に取り込む。左右の耳に分岐できるように引数をいれた. "L" or "R" を引数として使える
{
	signed short sb[DataNum];
	int j,i,k,tflg;
	FILE *fp,*fpcopy;
		
		if (receiver_side == "L"){
			fp =fopen("result_sensing/chk_rxwv_left.dat","w");
		}
		else{
			fp =fopen("result_sensing/chk_rxwv_right.dat","w");
		}
		
		if (NULL ==fp){
		fprintf(stderr, "fileopen error!");	 exit(7);
		}
		j = 0;
		i = 0;
		tflg = 0;
		
		for(k = 0;k < DataNum;k++)
			{	
				if (receiver_side == "L"){//*ここが左耳の条件　ここをかえよう
				sb[j]  = (buf[3*k]&0x00ff)|((buf[3*k+1]<<8)&0x0300);	// ch1
				j++;
				}
				else{//*ここが右耳の条件
				sb[j] = ((buf[3*k+1]>>2)&0x003f) |  ((buf[3*k+2]<<6)&0x03C0); // ch2 
				j++;
				}
					
				if ((tflg == 1) ||  ( (j > 15) && (sb[j-15] < TRIGTSD ) && (sb[j-10] > TRIGTSD ) &&    // to detect trigger override pulse
					(sb[j-9] > TRIGTSD ) && (sb[j-8] > TRIGTSD ) && (sb[j-1] < TRIGTSD ) ) )            //  ~10usec
					{
					rxwfm[i] = (double)(sb[j-1] - 0x0200);
					if (flg_rwv == 1 )		
						fprintf(fp, "%f\n", rxwfm[i]);
//							fprintf(fpcopy, "%f\n", rxwfm[i]);
					i++;
					tflg = 1;
					if ( i >= BOXNUM )		break;
					}
			}
		fclose(fp);
//		fclose(fpcopy);
//		rename(file,filenew); //move fpcopy to diferrent directry
	}

void fft_rx(void )    // perform FFT of rx wavefrom; result contaned to struct CMPLX rxspc[BOXNUM]
	{
	int		i, log2_N, N, ret, mb;
 	struct GPU_FFT_COMPLEX *base;
 	struct GPU_FFT *fft;
	FILE *fp;
	
	log2_N = 15; // for 32768pts
	N = 1<<log2_N; // FFT length
	mb = mbox_open();

   ret = gpu_fft_prepare(mb, log2_N, GPU_FFT_FWD, 1, &fft); 
	base = fft->in; // input buffer
	
	for (i=0; i<BOXNUM; i++)	{
		base[i].re = rxwfm[i];		base[i].im  = 0.;		}

	gpu_fft_execute(fft);	
	base = fft->out;

	for (i=0; i<BOXNUM; i++)	{
		rxspc[i].re = (double)base[i].re;	
		rxspc[i].im = (double)base[i].im;	// no change
		}
	if (flg_rsp == 1)
		{	
		if (NULL == ( fp=fopen("result_sensing/chk_rxsp.dat","w")))
			{	fprintf(stderr, "fileopen error!");	exit(0);	}
			for (i=0; i< BOXNUM; i++)		
				fprintf(fp, "%f\n", sqrt(pow(base[i].re, 2) + pow(base[i].im, 2)));
			fclose(fp);
		}
	gpu_fft_release(fft); // Videocore memory lost if not freed !
	mbox_close(mb);

   return;
	}

void ifft_mx(void )    // perform IFFT of muutipled spectrum; result contaned to struct CMPLX crwfm[BOXNUM]
	{
	int		i, log2_N, N, ret, mb;
 	struct GPU_FFT_COMPLEX *base;
 	struct GPU_FFT *fft;
	FILE *fp;
//	int FILTNUM;
//	double cutoff = 20000.;     // Digital low pass filtering  in Hz
//	FILTNUM = (int)( cutoff * (double)(BOXNUM/FS));	    //  Fc /  unit freq ( = (Fsample / window length) ) 
	log2_N = 15; // for 32768pts
	N = 1<<log2_N; // FFT length
	mb = mbox_open();

   ret = gpu_fft_prepare(mb, log2_N, GPU_FFT_REV, 1, &fft); 
	base = fft->in; // input buffer
	
	for (i=0; i<BOXNUM; i++)	{
		base[i].re = mxspc[i].re;		base[i].im  = mxspc[i].im;
//		if ( (i < FILTNUM + 1 ) || (i > BOXNUM-FILTNUM +1 ) )			{
//			base[i].re = 0.;		base[i].im  = 0.;	}
		}

	gpu_fft_execute(fft);	
	base = fft->out;
	
	for (i=0; i<BOXNUM; i++)	{
		crwfm[i].re = (double)base[i].re / (double)N;	
		crwfm[i].im = (double)base[i].im / (double)N;
		}

	gpu_fft_release(fft); // Videocore memory lost if not freed !

	ret = gpu_fft_prepare(mb, log2_N, GPU_FFT_REV, 1, &fft); 
	base = fft->in; // input buffer
	
	for (i=0; i<BOXNUM; i++)	{
		base[i].re = mxspc2[i].re;		base[i].im  = mxspc2[i].im;

//		if ( (i < FILTNUM + 1 ) || (i > BOXNUM-FILTNUM +1 ) )			{
//			base[i].re = 0.;		base[i].im  = 0.;	}

	}
	gpu_fft_execute(fft);	
	base = fft->out;

	for (i=0; i<BOXNUM; i++)	{
		crwfm2[i].re = (double)base[i].re / (double)N;	
		crwfm2[i].im = (double)base[i].im / (double)N;
		}
		
	gpu_fft_release(fft); // Videocore memory lost if not freed !
	mbox_close(mb);
   
	return;
	}

void cal_env(char* receiver_side)//相互相関波形を左右の耳で保存するために引数で区別
	{
	FILE *fp;
	int i;
    char filename[64];
    
    if (receiver_side == "L") {
		sprintf(filename,"result_sensing/chk_crwv_left.dat");
	}
	else{
		sprintf(filename,"result_sensing/chk_crwv_right.dat");
	}
	
	if (NULL == ( fp=fopen(filename,"w")))//ここをright leftで切り替え？
		{	fprintf(stderr, "fileopen error!");		exit(0);	}

		for (i=0; i< BOXNUM; i++)		{	
		crwv[i] = sqrt(crwfm[i].re * crwfm[i].re + crwfm2[i].re * crwfm2[i].re	);

				fprintf(fp, "%f\n", crwv[i]);			}

	fclose(fp);
	}

void oc_lms(double *xpp, double *ypp, int k, double *yy)
{
double a, b, c, det, x2[3], x[3];
double *y;
int		i;

y = yy;

x[0] = (double)(k-1);
x[1] = (double)k;
x[2] = (double)(k+1);

for (i=0; i<3; i++)
	x2[i] = x[i] * x[i];

det = x2[0]*x[1] + x[0]*x2[2] + x2[1]*x[2] - x[1]*x2[2] - x2[0]*x[2] - x[0]*x2[1];
a = (x[1]-x[2])*y[0] -(x[0]-x[2])*y[1] + (x[0]-x[1])*y[2];		
b = -(x2[1]-x2[2])*y[0] + (x2[0]-x2[2])*y[1] - (x2[0]-x2[1])*y[2];
c = (x2[1]*x[2]-x[1]*x2[2])*y[0] - (x2[0]*x[2]-x[0]*x2[2])*y[1] + (x2[0]*x[1]-x[0]*x2[1])*y[2];
a /= det; 	b /= det; c /= det;

*xpp = -b/2./a;
*ypp = -(b*b/4./a)+c;

return;
}



void extract_peaks(char* receiver_side)							//左右のピークを区別するために引数を導入
	{
	int i,maxflag;
	double 	max=0.,thd, xp, yp;
	FILE *fp;
	
	if (NULL == ( fp=fopen("result_sensing/peaks.dat","a")))
			{	fprintf(stderr, "fileopen error!");	exit(0);	}
//	for (i=0; i<BOXNUM/2; i++)		{								//# BOXNUM to BOXNUM/2 →　directpulsetimeに変更　直達波になるように設定
	for (i=0; i<direct_pulse_time; i++)	{
					if (max < crwv[i] ){
						max = crwv[i]; 
						maxflag=i;}	
	}
	printf("maxvalue %d us %lf\n", maxflag,max );		
	thd = max * thd_limit;
//	printf("thresh value %lf \n", thd);
	for (i=1; i<BOXNUM/2-1; i++)		{							//# BOXNUM to BOXNUM/2
	if ( ( (crwv[i]-crwv[i-1]) > 0.) && ( (crwv[i+1]-crwv[i]) < 0.)  && ( crwv[i] > thd )  && (i > direct_pulse_time ))	//#
		{
		oc_lms(&xp, &yp, i, &crwv[i-1]);

		if (receiver_side == "L"){
			leftpeaks[lflag] = (int)xp;	
			leftpeaks_val[lflag] = yp/max;	
		//	printf("left %d us\n", leftpeaks[lflag]);				
			lflag++;
		}
		else{
			rightpeaks[rflag] = (int)xp;
			rightpeaks_val[rflag] = yp/max;	
		//	printf("right %d us\n", rightpeaks[rflag]);			
			rflag++;
		}
		fprintf(fp,"%f\t%e\t%lf\n", xp, yp,yp/max);
		}
	}
	fclose(fp);
	}

void make_mxspc(void)
{
	int i;
	FILE *fp;
	for (i=0; i<BOXNUM; i++)
	{	mxspc[i].re = txspc[i].re * rxspc[i].re - txspc[i].im * rxspc[i].im;
		mxspc[i].im = txspc[i].im * rxspc[i].re + txspc[i].re * rxspc[i].im;

		// For Hilbert pair
		if ( i <= BOXNUM/2 )	{
			mxspc2[i].re = mxspc[i].im;	mxspc2[i].im = -mxspc[i].re;	}
		else						{
			mxspc2[i].re = -mxspc[i].im;	mxspc2[i].im = mxspc[i].re;	}
	}
		if (flg_crsp ==1 )			{
				if (NULL == ( fp=fopen("result_sensing/chk_crsp.dat","w")))
					{		fprintf(stderr, "fileopen error!");	 exit(7);	}
			for (i=0; i<BOXNUM; i++)
				fprintf(fp, "%f\n", sqrt(mxspc[i].re*mxspc[i].re + mxspc[i].im*mxspc[i].im));
			fclose(fp);
			}
}

void pre_thread2(void )
{
	UCHAR BitMode;

      ftstatus = FT_Open(0, &ftHandle);
      if (ftstatus != FT_OK)
      {
        printf("Error: FT_Open\n");
      }
 
      ftstatus = FT_Purge(ftHandle, FT_PURGE_RX | FT_PURGE_TX);
      if(ftstatus != FT_OK){
    	printf("Error: FT_Purge NG\n");
      }

      ftstatus = FT_SetBitMode(ftHandle, 0x00, 0x40);  
      if(ftstatus != FT_OK){
    	printf("Error: FT_SetBitMode(Sync-FIFO Mode)\n");
      }

      ftstatus = FT_SetTimeouts(ftHandle, 5000, 5000);
      if(ftstatus != FT_OK){
    	printf("Error: FT_SetTimeouts\n");
      }
      
      ftstatus = FT_SetLatencyTimer(ftHandle, 50);
      if(ftstatus != FT_OK){
    	printf("Error: FT_SetLatencyTimer\n");
      }

      ftstatus = FT_SetUSBParameters(ftHandle, 65536, 65536);
      if(ftstatus != FT_OK){
    	printf("Error: FT_SetUSBParameters\n");
      }

}
 
void post_thread2(void )
{
      ftstatus = FT_Close(ftHandle);
      if(ftstatus != FT_OK){
    	printf("Error: FT_Close\n");
      }
  //    else {
  //  	printf("OK: FT_Close\n");
  //    }

}

void* thread1(void* pParam)
{
	system("aplay dc192kwv.wav 2> .aaa");
}
void* thread1_short(void* pParam)
{
	system("aplay dc192kwv_short.wav 2> .aaa");
}


void* thread2(void* pParam)
{
	int ret;
    digitalWrite(GPIO25, 0); // Start Trigger assert L
    delay(1);

    // FTDI IC FIFO Clear
    ftstatus = FT_Purge(ftHandle, FT_PURGE_RX | FT_PURGE_TX);
    if(ftstatus != FT_OK){
    	printf("Error: FT_Purge NG\n");
    }
   // else {
   // 	printf("OK: FT_Purge OK\n");
   // }

  // delay(0.1);     
     // Start Trigger assert H
    digitalWrite(GPIO25, 1);        
    delay(0.1);    
    
    ret = -1;
    
    for(;;){
             
    ftstatus = FT_GetStatus(ftHandle, &RxBytes1, &TxBytes, &EventDWord);
    
     if(ftstatus != FT_OK){
    	  printf("Error: FT_GetStatus NG\n");
    	    break;
    }
    else {
    	    if(RxBytes1 == 0){
    	      continue;
    	    }
    }
      
      // USB Read1      
      ftstatus = FT_Read(ftHandle,buf1,FByte, &BytesReceived);
      if(ftstatus != FT_OK){
       	printf("Error: FT Read\n");
      }
     // else {
     //   if(BytesReceived == FByte) {
     //  printf("OK: FT Read\n");
     //   }
     // }
      
      // USB Read2      
      ftstatus = FT_Read(ftHandle,buf2,FByte, &BytesReceived);
      if(ftstatus != FT_OK){
       	printf("Error: FT Read\n");
      }


      // USB Read3        
      ftstatus = FT_Read(ftHandle,buf3,FByte, &BytesReceived);
      if(ftstatus != FT_OK){
       	printf("Error: FT Read\n");
      }


      // USB Read4
      ftstatus = FT_Read(ftHandle,buf4,FByte, &BytesReceived);
      if(ftstatus != FT_OK){
       	printf("Error: FT Read\n");
      }
      
      ret = 0;
      break;
      }
    
}

void param_read(void )
	{
	FILE *fp;
	char 	buf[BUFLEN];

	if( NULL == (fp = fopen("sensing.prm","r")) )
		{	fprintf(stderr, "Cannot open data file\n");	exit(0);	}
		fgets(buf, BUFLEN, fp);	nrep = atoi(buf);
		fgets(buf, BUFLEN, fp);	flg_twv = atoi(buf);
		fgets(buf, BUFLEN, fp);	flg_tsp = atoi(buf);
		fgets(buf, BUFLEN, fp);	flg_rwv = atoi(buf);
		fgets(buf, BUFLEN, fp);	flg_rsp = atoi(buf);
		fgets(buf, BUFLEN, fp);	flg_crwv = atoi(buf);
		fgets(buf, BUFLEN, fp);	flg_crsp = atoi(buf);
		fgets(buf, BUFLEN, fp);	flg_peak = atoi(buf);

	fclose(fp);
/*
		fprintf(stderr, "%d\t\t Number of repetition\n", nrep);
		fprintf(stderr, "%d\t\t tx wfm out\n", flg_twv);
		fprintf(stderr, "%d\t\t tx spectrum out\n", flg_tsp);
		fprintf(stderr, "%d\t\t rx wfm out\n", flg_rwv);
		fprintf(stderr, "%d\t\t rx spectrum out\n", flg_rsp);
		fprintf(stderr, "%d\t\t CCR wfm out\n", flg_crwv);
		fprintf(stderr, "%d\t\t CCR spectrum out\n", flg_crsp);
		fprintf(stderr, "%d\t\t peaks out\n", flg_peak);
*/
	}

int main(int argc, char *argv[]){

	signed short sb[DataNum];
	pthread_t tid1, tid2;
	int ch, k, j, i, tflg;
	FILE *gp,*gp2, *fp,*fpcopy;
	int failflag = 0;
	int failcount=0;
	
	//コマンドライン引数の読み込//////
	trialnum=atoi(argv[1]);
	multipulseflag=atoi(argv[2]);
        neckflag = atoi(argv[3]);
	duration_short_ratio =atoi(argv[4]);
	//ここまで/////////////////////
	
	wiringPiSetupGpio();
	pinMode(GPIO25, OUTPUT);
	digitalWrite(GPIO25, 0);
	sleep(1); // Driving waveform generation


	param_read();
	tx_wfm_load(duration_short_ratio);
	fft_tx();

//	gp = popen("gnuplot -persist","w");
//	gp2 = popen("gnuplot -persist","w");
//	fprintf(gp2, "set yr [-500:500]\n");

	while (failflag ==0 ){ //failflag が0の間はずっと繰り返す
		failflag =1;
 //       system("sudo rm peaks.dat");
		printf("sensing start \n"); 
		fprintf(stderr, "fail count %d \n", failcount);
	
		for (k=0; k< BOXNUM; k++){
			leftpeaks[k] = 0;
			leftpeaks_val[k] = 0;
			rightpeaks[k] = 0;
			rightpeaks_val[k] = 0;
			} 
		lflag=0;
		rflag=0;
//		char rx[64];
//		sprintf(rx,"rx%d.dat",ch);
//		char file[128];
//		char filenew[128];
//		sprintf(file,"/home/pi/projects3/pcm/rx%d.dat",ch);
//		sprintf(filenew,"/home/pi/projects3/pcm/koko/rx%d.dat",ch);


		pre_thread2();
		pthread_create(&tid2, NULL, thread2, NULL);  // ADC storage start
		usleep(50000);								// to gain margin GPIO Low-->High to ADC start
		if(duration_short_ratio==1)
			{pthread_create(&tid1, NULL, thread1, NULL);}	// drive signal start}
		else
			{pthread_create(&tid1, NULL, thread1_short, NULL);}	// drive signal start}
		pthread_join(tid1, NULL);
		pthread_join(tid2, NULL);
		digitalWrite(GPIO25, 0);  
		post_thread2();
		
		for (int x=0; x< FByte ;x++) {
		  buf[x]          = buf1[x];
		  buf[x+FByte]    = buf2[x];
		  buf[x+FByte*2]  = buf3[x];
		  buf[x+FByte*3]  = buf4[x];			  
		}
		make_rx("R");
	
///////////////////////ここからファイルの破損や欠陥がないか確認,片耳が問題なければもう一方はしない/////////////////////

	int overdet = 0, emptydat=0;
	for (k=0; k< BOXNUM; k++)
		{
		if ( rxwfm[k] > 509 ) overdet++;
		if ( rxwfm[k] == 0) emptydat++;						
		}
	if (overdet > BOXNUM/10 || emptydat > BOXNUM/2 -1 ){
		printf("failed detact\n");
		failflag = 0;
		failcount++;
		if (failcount > failmax) 			break;
		if (failcount < failmax)			continue; //while の先頭に戻る
	}

///////////////////////ここまで///////////////////////////////////////////////
		fft_rx();
		make_mxspc();
		ifft_mx();
		cal_env("R");
		extract_peaks("R");
		
		make_rx("L");
		fft_rx();
		make_mxspc();
		ifft_mx();
		cal_env("L");
		extract_peaks("L");
	
///////////////////両受信した後に破損がないか最終チェック//////////////////	
	if ((lflag > peaksmax || rflag > peaksmax) || (lflag == 0 && rflag == 0)){
//	if ((lflag > peaksmax || rflag > peaksmax) ){
		printf("\ntoo much peaks or can't detect any peaks\n");
		failflag = 0;
		failcount++;
		if (failcount > failmax) 			break;
		if (failcount < failmax)			continue;
	}
///////////////////ここまで//////////////////	///////////////////////////


//	fprintf(gp, "set xr[0:15000]\n");
//	if (flg_crwv == 1){
//		fprintf(gp, "plot \"result_sensing/chk_crwv_left.dat\" using 0:1 with lines linetype 7\n");   //#
//		fprintf(gp, "replot \"result_sensing/chk_crwv_right.dat\" using 0:1 with lines linetype 3 \n");
//	}
//	if (flg_peak == 1)	
//			fprintf(gp, "replot \"result_sensing/peaks.dat\" using 1:2 linetype 3\n");

//	fflush(gp);

//	fprintf(gp2, "plot \"result_sensing/chk_rxwv.dat\" with lines\n");
//	fflush(gp2);
	printf("right and left peaks = %d, %d \n",rflag,lflag);
	}
	
//	pclose(gp2);	
	
	local_cal(lflag,rflag,leftpeaks,leftpeaks_val,rightpeaks,rightpeaks_val,trialnum,multipulseflag,neckflag,duration_short_ratio ); //make location data
 


// pclose(gp);	
}






