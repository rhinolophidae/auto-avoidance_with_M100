
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <usb.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <wiringPi.h>
#include <Python.h>

#include "mailbox.h"
#include "gpu_fft.h"
#include "local_avdeg_cal.h"
#include "ftd2xx.h"
// 10m/340m = 29.4msec
// 1MHz sample
// 29.4x(0.001)x1000000 = 29.4kpts
// 

#define BOXNUM  65536  //  data points for calculating array
#define DataNum 65536
//#define BOXNUM  32768  //  data points for calculating array
//#define DataNum 32768
#define FByte DataNum*3/4 	
#define MAX_MEAS_NUM 600
#define TRIGTSD	960
#define FS 1.e6        //  sampling frequency
#define BUFLEN 256

#define TIMEOUT (5*1000)
#define EP6	0x86

#define direct_pulse_time 1000 
#define failmax  3 
#define peaksmax  50 
#define thd_limit  0.4 

#define GPIO25 25 // meas start
#define GPIO8  8 // reset
#define GPIO4  4 // FPGA setting data[4]
#define GPIO17 17 // FPGA setting data[3]
#define GPIO27 27 // FPGA setting data[2]
#define GPIO22 22 // FPGA setting data[1]
#define GPIO10 10 // FPGA setting data[0]
#define GPIO9  9 // FPGA setting latch signal(ADC receive point)
#define GPIO11 11 // FPGA setting latch signal(measurement num)
#define GPIO5  5 // FPGA setting latch signal(measurement time/one measurement)
#define GPIO6  6 // Select USB Pulse Data(thermo or not)

unsigned char buf1[MAX_MEAS_NUM][FByte];
unsigned char buf2[MAX_MEAS_NUM][FByte];
unsigned char buf3[MAX_MEAS_NUM][FByte];
unsigned char buf4[MAX_MEAS_NUM][FByte];
unsigned char buf[MAX_MEAS_NUM][FByte*4];

FT_HANDLE ftHandle;
FT_STATUS ftstatus;
DWORD BytesWritten;
DWORD EventDWord;
DWORD RxBytes1;
DWORD TxBytes;
DWORD BytesReceived;
WORD adc_1[FByte];
WORD adc_2[FByte];

struct CMPLX {	double re, im;	};
float rxspc_pw[BOXNUM], mxspc_pw[BOXNUM];
struct CMPLX txspc[BOXNUM], rxspc[BOXNUM], mxspc[BOXNUM], crwfm[BOXNUM], 
				mxspc2[BOXNUM], crwfm2[BOXNUM];
int leftpeaks[BOXNUM],rightpeaks[BOXNUM];
int dist[BOXNUM],direc[BOXNUM];
double leftpeaks_val[BOXNUM],rightpeaks_val[BOXNUM];

struct usb_bus *sbus;
struct usb_device *sdev;
usb_dev_handle *sdh;

void* thread1(void* pParam);  // HiFiBerry DAC wav file output
void* thread1_short(void* pParam);
void* thread2(void* pParam);  // FX2 USB  ADC data input

double txwfm[BOXNUM], rxwfm[BOXNUM], crwv[BOXNUM];		// tx & rx waveform with 1MHz sample
char   TxBuffer[BOXNUM];
int    TxBuffer_size;

int adc_rec_setting, mes_tim_setting, mes_num_setting;
int adc_rec_num, mes_tim, mes_num;
int f0, f1, T,BCC, twait;

void tx_wfm_load()     // load tx waveform file(pulse drive signal) pulse_drive.dat 
	{
	FILE *fp;
	int	i;
	char  cbuf[255];
	char filename[64];
	int tmp0,tmp1,tmp2,tmp3;
	
	for (i=0; i< BOXNUM; i++)		txwfm[i] = 0;

	sprintf(filename, "pulse_drive_data/pulse_drive_%d_%d_%d.dat", f0, f1, T);
	if (NULL == ( fp=fopen(filename,"r")))
		{	fprintf(stderr, "fileopen error!1");	exit(2);	}

	i = 0;
	while(NULL != fgets(cbuf,255, fp)){
		txwfm[i] = 2.0 * atof(cbuf) - 1.0;

		if(i%4 == 0)
      		tmp0 = atoi(cbuf);
		if(i%4 == 1)
      		tmp1 = atoi(cbuf);
		if(i%4 == 2)
      		tmp2 = atoi(cbuf);
		if(i%4 == 3)
      		tmp3 = atoi(cbuf);

		if(i%4 == 3) {
		   sprintf( &TxBuffer[i/4], "%X", tmp3*8+tmp2*4+tmp1*2+tmp0);
		}

		i++;
	}
	TxBuffer_size = ceil((double)(i)/4);

	fclose(fp);
	
	fprintf(stderr, "result_sensing/%d_%d_%d_%d_%d/chk_txwv_emit.dat", f0, f1, T, BCC, twait);

	sprintf(filename, "result_sensing/%d_%d_%d_%d_%d/chk_txwv_emit.dat", f0, f1, T, BCC, twait);
	if (NULL == ( fp=fopen(filename,"w"))){
		fprintf(stderr, "fileopen error!2");
		exit(0);
	}
	for (i=0; i< BOXNUM; i++){
		fprintf(fp, "%f\n", txwfm[i]);
	}
	fclose(fp);
	return;
}


void pulse_aux_load()     //
	{
	FILE *fp;
	int	i;
	char  cbuf[255];
	int tmp0,tmp1,tmp2,tmp3;
	char filename[64];

	sprintf(filename, "pulse_drive_data/pulse_drive_aux_%d_%d_%d.dat", f0, f1, T);
	if (NULL == ( fp=fopen(filename,"r")))
		{	fprintf(stderr, "fileopen error!3");	exit(2);	}

	i = 0;
	while ( NULL != fgets(cbuf,255, fp) ){
		if(i%4 == 0)
      		tmp0 = atoi(cbuf);
		if(i%4 == 1)
      		tmp1 = atoi(cbuf);
		if(i%4 == 2)
      		tmp2 = atoi(cbuf);
		if(i%4 == 3)
      		tmp3 = atoi(cbuf);

		if(i%4 == 3) {
		   sprintf( &TxBuffer[i/4], "%X", tmp3*8+tmp2*4+tmp1*2+tmp0);
		}

		i++;
	}
	TxBuffer_size = ceil((double)(i)/4);
	fclose(fp);
	return;
}


void ref_tx_wfm_load()     // load tx waveform file(pulse drive signal) pulse_drive.dat 
	{
	FILE *fp;
	int	i;
	char  cbuf[255];
	char filename[64];
	int tmp0,tmp1,tmp2,tmp3;
	
	for (i=0; i< BOXNUM; i++)		txwfm[i] = 0;

	fprintf(stderr, "result_sensing/%d_%d_%d_%d_%d/chk_txwv_ref.dat", f0, f1, T, BCC, twait);
	sprintf(filename, "result_sensing/%d_%d_%d_%d_%d/chk_txwv_ref.dat", f0, f1, T, BCC, twait);
	if (NULL == ( fp=fopen(filename,"r")))
		{	fprintf(stderr, "fileopen error!1");	exit(2);	}

	i = 0;
	while(NULL != fgets(cbuf,255, fp)){
		txwfm[i] = atof(cbuf);
		i++;
	}
	fclose(fp);	
	return;
}

void fft_tx(void )    // perform FFT & conjugate of tx wavefrom; result contaned to struct CMPLX txspc[BOXNUM]
	{
	int		i, log2_N, N, ret, mb;
 	struct GPU_FFT_COMPLEX *base;
 	struct GPU_FFT *fft;
	FILE *fp;
	char filename[64];
	
	log2_N = 16; // for 65536pts
//	log2_N = 15; // for 32768pts
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

	sprintf(filename, "result_sensing/%d_%d_%d_%d_%d/chk_txspc_by_c.dat", f0, f1, T, BCC, twait);
	if (NULL == ( fp=fopen(filename,"w")))
		{	fprintf(stderr, "fileopen error!");	exit(3);	}
	for (i=0; i< BOXNUM; i++)		
			fprintf(fp, "%f\n", sqrt(pow(base[i].re, 2) + pow(base[i].im, 2)));
	fclose(fp);

	gpu_fft_release(fft); // Videocore memory lost if not freed !
	mbox_close(mb);
   return;
	}


void make_rx(char* receiver_side, int mes_index, double rxwfm[BOXNUM], int loop_n, int year,int month,int day,int hour,int mint,int sec,int us)
{
	signed short sb[DataNum];
	int j,i,k;
	FILE *fp,*fpcopy;
	char filename[256];
	float rx_ave;
		
		
		if (receiver_side == "L"){
			sprintf(filename, "result_sensing/%d_%d_%d_%d_%d/rxwv/chk_rxwv_left_%d.dat", f0, f1, T, BCC, twait, loop_n);
			fp =fopen(filename, "w");
		}
		else{
			sprintf(filename, "result_sensing/%d_%d_%d_%d_%d/rxwv/chk_rxwv_right_%d.dat", f0, f1, T, BCC, twait, loop_n);
			fp =fopen(filename, "w");
		}
		
		if (NULL ==fp){
		fprintf(stderr, "fileopen error!6");	 exit(7);
		}
		
		
		setvbuf(fp, NULL, _IOFBF, 512*1024);	
		j = 0;
		i = 0;
		/////////////////// data save /////////////////////////////////////////////////////
		fprintf(fp, "Date: %d/%02d/%02d %02d:%02d:%02d.%06d\n",   
			year, month, day, hour, mint, sec, us);
		////////////////////////////////////////////////////////////////////////////////
		
		rx_ave=0;
		for(k = 0;k < DataNum;k++)
			{	
				if (receiver_side == "L"){
					sb[j]  = (buf[mes_index][3*k]&0x00ff)|((buf[mes_index][3*k+1]<<8)&0x0300);	// ch1
					j++;
				}
				else{
					sb[j] = ((buf[mes_index][3*k+1]>>2)&0x003f) |  ((buf[mes_index][3*k+2]<<6)&0x03C0); // ch2 
					j++;
				}
				rxwfm[i] = (double)(sb[j-1] - 0x0200);
								
				
				if (receiver_side == "R"){no DC cut
					rxwfm[i] = rxwfm[i]-16.3;//-14.0
				} else if(receiver_side == "L"){
					rxwfm[i] = rxwfm[i]-1.2;
				}
				
				
				///////////////////data save/////////////////////////////////////////////////////
				
				if (receiver_side == "L"){
					fprintf(fp, "%f\n", rxwfm[i]);
				}
				if (receiver_side == "R"){
					fprintf(fp, "%f\n", rxwfm[i]);
				}				
				////////////////////////////////////////////////////////////////////////////////
				rx_ave=rx_ave+rxwfm[i]/ adc_rec_num;
				
				i++;
				if ( i >= adc_rec_num )		break;
			}
		for (i = 0; i < adc_rec_num;i++){
			rxwfm[i]=rxwfm[i]-rx_ave;
		}
						
		fclose(fp);
	}


void fft_rx(double rxwfm[BOXNUM], float rxspc_pw[BOXNUM])    // perform FFT of rx wavefrom; result contaned to struct CMPLX rxspc[BOXNUM]
	{
	int		i, log2_N, N, ret, mb;
 	struct GPU_FFT_COMPLEX *base;
 	struct GPU_FFT *fft;
//	FILE *fp;
	
	log2_N = 16; // for 65536pts
//	log2_N = 15; // for 32768pts
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
		rxspc_pw[i] = sqrt(pow(base[i].re, 2) + pow(base[i].im, 2));
		}
/*	if (NULL == ( fp=fopen("result_sensing/chk_rxsp.dat","w")))
		{	fprintf(stderr, "fileopen error!");	exit(0);	}
		for (i=0; i< BOXNUM; i++){
			fprintf(fp, "%f\n", rxspc_pw[i]);
		}
		fclose(fp);
*/			
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
	log2_N = 16; // for 65536pts
//	log2_N = 15; // for 32768pts
	N = 1<<log2_N; // FFT length
	mb = mbox_open();

   ret = gpu_fft_prepare(mb, log2_N, GPU_FFT_REV, 1, &fft); 
	base = fft->in; // input buffer
	
	for (i=0; i<BOXNUM; i++)	{
		base[i].re = mxspc[i].re;		base[i].im  = mxspc[i].im;
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


void cal_env(char* receiver_side, double crwv[BOXNUM])
	{
//	FILE *fp;
	int i;
//    char filename[64];
/*    
    if (receiver_side == "L") {
		sprintf(filename,"result_sensing/chk_crwv_left.dat");
	}else{
		sprintf(filename,"result_sensing/chk_crwv_right.dat");
	}
	
	if (NULL == ( fp=fopen(filename,"w")))
		{	fprintf(stderr, "fileopen error!");		exit(0);	}
*/
	for (i=0; i< BOXNUM; i++)		{	
		crwv[i] = sqrt(crwfm[i].re * crwfm[i].re + crwfm2[i].re * crwfm2[i].re	);
//		fprintf(fp, "%f\n", crwv[i]);
		}
//	fclose(fp);
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


void make_mxspc(float mxspc_pw[BOXNUM])
{
	int i;
	FILE *fp;
	for (i=0; i<BOXNUM; i++)
	{	mxspc[i].re = txspc[i].re * rxspc[i].re - txspc[i].im * rxspc[i].im;
		mxspc[i].im = txspc[i].im * rxspc[i].re + txspc[i].re * rxspc[i].im;
		mxspc_pw[i]=sqrt(mxspc[i].re*mxspc[i].re + mxspc[i].im*mxspc[i].im);

		// For Hilbert pair
		if ( i <= BOXNUM/2 )	{
			mxspc2[i].re = mxspc[i].im;	mxspc2[i].im = -mxspc[i].re;	}
		else						{
			mxspc2[i].re = -mxspc[i].im;	mxspc2[i].im = mxspc[i].re;	}
	}
/*	if (NULL == ( fp=fopen("result_sensing/chk_crsp.dat","w")))
		{		fprintf(stderr, "fileopen error!");	 exit(7);	}
	for (i=0; i<BOXNUM; i++){
		fprintf(fp, "%f\n", sqrt(mxspc[i].re*mxspc[i].re + mxspc[i].im*mxspc[i].im));
	}
	fclose(fp);
*/
}


int extract_peaks(char* receiver_side, int rightpeaks[BOXNUM], int leftpeaks[BOXNUM])							
	{
	int i,maxflag, t_max, t_min, alpha, twL_latest_id, twR_latest_id, t_width;
	double 	thd, thd_base, xp, yp, t_rest, sigma;
//	FILE *fp;
	
	int lflag=0, rflag=0; // peaks number

	thd_base=150000; //minimum_value_for_threshold(recomended: 130000-165000 (2021/12/15))
	t_min=1000;//us (minimum_det_range_time)
	t_max=25000;//us (minimum_det_range_time)
	t_width=700; //separation_time_window (cyokuzen no ikichi goe index kara nanbyou keika shite irunoka de separate)
	
	alpha=200000; //(2021/12/15 fitted) 
	sigma=BOXNUM / 35;
	
	int max1(int a, int b) { return a > b ? a : b; }
	
	twL_latest_id=0;
	twR_latest_id=0;
	for (i=t_min; i<t_max; i++){							//# BOXNUM to BOXNUM/2
		thd = max1(thd_base,  alpha*exp(-i/sigma));
//	printf("thresh value %lf \n\n", thd);
		if ( crwv[i]-crwv[i-1] > 0. && crwv[i+1]-crwv[i] < 0. && crwv[i] > thd )	//#
			{
			oc_lms(&xp, &yp, i, &crwv[i-1]); //nazo
			if (receiver_side == "L"){
				if (lflag >= 1 && i-twL_latest_id<=t_width){
					if(crwv[i]  > leftpeaks_val[lflag-1]){
						leftpeaks[lflag-1] = i;
						leftpeaks_val[lflag-1] = crwv[i];
						printf("updt_left %d us\n", leftpeaks[lflag-1]);
						twL_latest_id=i;
					}
				}else{
					leftpeaks[lflag] = i;	
					leftpeaks_val[lflag] = crwv[i];	
					printf("left %d us\n", leftpeaks[lflag]);				
					lflag++;
					twL_latest_id=i;
				}
			}else{
				if (rflag >= 1 && i-twR_latest_id<=t_width){
					if(crwv[i] > rightpeaks_val[rflag-1]){
						rightpeaks[rflag-1] = i;
						rightpeaks_val[rflag-1] = crwv[i];
						printf("updt_right %d us\n", rightpeaks[rflag-1]);
						twR_latest_id=i;
					}
				}else{
					rightpeaks[rflag] = i;
					rightpeaks_val[rflag] = crwv[i];	
					printf("right %d us\n", rightpeaks[rflag]);			
					rflag++;
					twR_latest_id=i;
				}
			}
//		fprintf(fp,"%f\t%e\t%lf\n", xp, yp,yp);
		}
			
	}
//	fclose(fp);
	printf("\n");
	if (receiver_side == "L"){ return lflag; }
	if (receiver_side == "R"){ return rflag; }

}


void pre_thread2(void ) // USB controller initialize
{
	UCHAR BitMode;

      ftstatus = FT_Open(0, &ftHandle);
      if (ftstatus != FT_OK)
      {
        printf("Errorfff: FT_Open\n");
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

      ftstatus = FT_SetUSBParameters(ftHandle, 65536, 0);
      if(ftstatus != FT_OK){
    	printf("Error: FT_SetUSBParameters\n");
      }

}
 
void post_thread2(void ) // USB contrller Handle Close
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
	int i;
  int *mes_num = (int *)pParam;

    // FTDI IC FIFO Clear
    ftstatus = FT_Purge(ftHandle, FT_PURGE_RX | FT_PURGE_TX);
    if(ftstatus != FT_OK){
    	printf("Error: FT_Purge NG\n");
    }
    else {
    	printf("OK: FT_Purge OK\n");
    }

    delay(0.1);
    digitalWrite(GPIO25, 1); // Measure Start
    delay(0.1);
    
    ret = -1;  
    for(;;){
             
    ftstatus = FT_GetStatus(ftHandle, &RxBytes1, &TxBytes, &EventDWord);
    //printf("mes_num=%d\n",mes_num);
    if(ftstatus != FT_OK){
    	  printf("Error: FT_GetStatus NG\n");
    	    break;
    }
    else {
    	    if(RxBytes1 == 0){
    	      continue;
    	    }
    }

			for(i = 0; i<*mes_num; i++) {
        // USB Read1  
        ftstatus = FT_Read(ftHandle,buf1[i],FByte, &BytesReceived);
        if(ftstatus != FT_OK){
         	printf("Error: FT Read\n");

        }         
        // USB Read2      
        ftstatus = FT_Read(ftHandle,buf2[i],FByte, &BytesReceived);
        if(ftstatus != FT_OK){
         	printf("Error: FT Read\n");
        }
  
  
        // USB Read3        
        ftstatus = FT_Read(ftHandle,buf3[i],FByte, &BytesReceived);
        if(ftstatus != FT_OK){
         	printf("Error: FT Read\n");
        }
  
        // USB Read4
        ftstatus = FT_Read(ftHandle,buf4[i],FByte, &BytesReceived);
        if(ftstatus != FT_OK){
         	printf("Error: FT Read\n");
        }
			}
      ret = 0;
      break;
      }

    
}

void param_read_fpga(void ) {
	FILE *fp;
	char buf[BUFLEN];

	if( NULL == (fp = fopen("sensing_fpga.prm","r")) )
		{	fprintf(stderr, "Cannot open data file\n");	exit(0);	}
		    fgets(buf, BUFLEN, fp);	adc_rec_setting = atoi(buf);
        fgets(buf, BUFLEN, fp);	mes_tim_setting = atoi(buf);
        fgets(buf, BUFLEN, fp);	mes_num_setting = atoi(buf);

	fclose(fp);
	}

void gpio_init(void){
	wiringPiSetupGpio();
   	pinMode(GPIO25,OUTPUT);
   	pinMode(GPIO8,OUTPUT);
    pinMode(GPIO4,OUTPUT);
    pinMode(GPIO17,OUTPUT);
    pinMode(GPIO27,OUTPUT);
    pinMode(GPIO22,OUTPUT);
    pinMode(GPIO10,OUTPUT);
    pinMode(GPIO9,OUTPUT);
    pinMode(GPIO11,OUTPUT);
    pinMode(GPIO5,OUTPUT);
    pinMode(GPIO6,OUTPUT);			

	digitalWrite(GPIO25, 0);
	digitalWrite(GPIO8,  0);
	digitalWrite(GPIO4,  0);
	digitalWrite(GPIO17, 0);
	digitalWrite(GPIO27, 0);
	digitalWrite(GPIO22, 0);
	digitalWrite(GPIO10, 0);
	digitalWrite(GPIO9,  0);
	digitalWrite(GPIO11, 0);
	digitalWrite(GPIO5,  0);
	digitalWrite(GPIO6,  0);
}

int table_fpga_setting_1(int adc_rec_setting)

	{
  int adc_rec_num;

      switch(adc_rec_setting) {
        case 0  : adc_rec_num = 8; break;
        case 1  : adc_rec_num = 8; break;
        case 2  : adc_rec_num = 8; break;
        case 3  : adc_rec_num = 8; break;
        case 4  : adc_rec_num = 16; break;
        case 5  : adc_rec_num = 32; break;
        case 6  : adc_rec_num = 64; break;
        case 7  : adc_rec_num = 128; break;
        case 8  : adc_rec_num = 256; break;
        case 9  : adc_rec_num = 512; break;
        case 10 : adc_rec_num = 1024; break;
        case 11 : adc_rec_num = 2048; break;
        case 12 : adc_rec_num = 4096; break;
        case 13 : adc_rec_num = 8192; break;							
        case 14 : adc_rec_num = 16384; break;
        case 15 : adc_rec_num = 32768; break;
        default : adc_rec_num = 65536; break;
      }
      return adc_rec_num;
	}

int table_fpga_setting_2(int mes_num_setting) 
	{
  int mes_num;
  
    switch(mes_num_setting) {
        case 0  : mes_num = 1;   break;
        case 1  : mes_num = 2;   break;
        case 2  : mes_num = 20;  break;
        case 3  : mes_num = 40;  break;
        case 4  : mes_num = 60;  break;
        case 5  : mes_num = 80;  break;
        case 6  : mes_num = 100; break;
        case 7  : mes_num = 120; break;
        case 8  : mes_num = 140; break;
        case 9  : mes_num = 160; break;
        case 10 : mes_num = 180; break;
        case 11 : mes_num = 200; break;
        case 12 : mes_num = 220; break;
        case 13 : mes_num = 240; break;							
        case 14 : mes_num = 260; break;
        case 15 : mes_num = 280; break;
        case 16 : mes_num = 300; break;
        case 17 : mes_num = 320; break;
        case 18 : mes_num = 340; break;
        case 19 : mes_num = 360; break;							
        case 20 : mes_num = 380; break;
        case 21 : mes_num = 400; break;
        case 22 : mes_num = 420; break;
        case 23 : mes_num = 440; break;							
        case 24 : mes_num = 460; break;
        case 25 : mes_num = 480; break;
        case 26 : mes_num = 500; break;
        case 27 : mes_num = 520; break;
        case 28 : mes_num = 540; break;
        case 29 : mes_num = 560; break;							
        case 30 : mes_num = 580; break;
        case 31 : mes_num = 600; break;
        default : mes_num = 600; break;
      }
      return mes_num;
	}

void conv_dec2bin(int dec, int binary[5]){

  binary[0] = 0;
  binary[1] = 0;
  binary[2] = 0;
  binary[3] = 0;
  binary[4] = 0;

	int i;

  for(i=0; dec>0; i++){
    binary[i] = dec % 2;
    dec = dec / 2;

  }

}

int usb_send(void){

    ftstatus = FT_Purge(ftHandle, FT_PURGE_RX | FT_PURGE_TX);
    if(ftstatus != FT_OK){
    	printf("Error: FT_Purge NG\n");
    }
 
    ftstatus = FT_Write(ftHandle, TxBuffer, TxBuffer_size, &BytesWritten);
    if(ftstatus != FT_OK){
      printf("Error: FT_Write Error\n");
    }		

}
int read_pulse_n(){
  
    char buf[BUFLEN];
    FILE *fp;
    int stream_fd;
    int Number_of_pulses=0;
    
    while(Number_of_pulses == 0){
      
	fp =fopen("pulse_n.prm", "r");
	stream_fd = fileno(fp);
	
	if (NULL ==fp || stream_fd == -1){
		fprintf(stderr, "fileopen error!6\n");
		fprintf(stderr,"fp: %c\n", fp);
	} else {
		fgets(buf, BUFLEN, fp);	
		Number_of_pulses = atoi(buf);
	}
	fclose(fp);
    }
    fprintf(stdout, "\n");
    fprintf(stdout,"Number_of_pulses: %d\n", Number_of_pulses);
    fprintf(stdout, "\n");
    
	return Number_of_pulses;
}

/*
void send_message_write(char* send_peaks){

    FILE *fp;
    int stream_fd;
    
      
	fp =fopen("det_log.txt", "w");
	stream_fd = fileno(fp);

	if (NULL ==fp || stream_fd == -1){
		fprintf(stderr, "fileopen error!6\n");
	} else {
		fprintf(fp, "%d\n", send_peak*);
	fclose(fp);
	}
    fprintf(stdout, "write_message: %s \n", send_peaks*);
}
*/
int main(int argc, char *argv[]){
    sleep(1);
   
    struct timeval myTime; 
    struct tm *time_st, *time_et;

	pthread_t tid1, tid2;
    int gpio_setting[5];
	
	f0 = atoi(argv[1]);
	f1 = atoi(argv[2]);
	T = atoi(argv[3]);
	BCC = atoi(argv[4]);
	twait = atoi(argv[5]);
	

	//FPGA//
    param_read_fpga();
	adc_rec_num = table_fpga_setting_1(adc_rec_setting);
	mes_num     = table_fpga_setting_2(mes_num_setting);

    gpio_init();
	digitalWrite(GPIO8, 0);
	sleep(0.1);
	digitalWrite(GPIO8, 1); 
    sleep(0.1);

    conv_dec2bin(adc_rec_setting, gpio_setting);
	digitalWrite(GPIO10, gpio_setting[0]); // setting[0]
	digitalWrite(GPIO22, gpio_setting[1]); // setting[1]
	digitalWrite(GPIO27, gpio_setting[2]); // setting[2]
	digitalWrite(GPIO17, gpio_setting[3]); // setting[3]
	digitalWrite(GPIO4, gpio_setting[4]); // setting[4]
    sleep(0.1);

	digitalWrite(GPIO9, 0);
    sleep(0.1);
	digitalWrite(GPIO9, 1); // latch signal assert
    sleep(0.1);    

    conv_dec2bin(mes_tim_setting, gpio_setting);	
	digitalWrite(GPIO10, gpio_setting[0]); // setting[0]
	digitalWrite(GPIO22, gpio_setting[1]); // setting[1]
	digitalWrite(GPIO27, gpio_setting[2]); // setting[2]
	digitalWrite(GPIO17, gpio_setting[3]); // setting[3]
	digitalWrite(GPIO4, gpio_setting[4]); // setting[4]
    sleep(0.1);

	digitalWrite(GPIO5, 0);
    sleep(0.1);
 	digitalWrite(GPIO5, 1); // latch signal assert
    sleep(0.1);     

    conv_dec2bin(mes_num_setting, gpio_setting);
	digitalWrite(GPIO10, gpio_setting[0]); // setting[0]
	digitalWrite(GPIO22, gpio_setting[1]); // setting[1]
	digitalWrite(GPIO27, gpio_setting[2]); // setting[2]
	digitalWrite(GPIO17, gpio_setting[3]); // setting[3]
	digitalWrite(GPIO4, gpio_setting[4]); // setting[4]
    sleep(0.1);

	digitalWrite(GPIO11, 0);
    sleep(0.1);
	digitalWrite(GPIO11, 1); // latch signal assert
    sleep(0.1);   

	pre_thread2(); // USB Controller Initialize

        tx_wfm_load(f0, f1, T);
	usb_send();
        sleep(0.1);

	pulse_aux_load();
	digitalWrite(GPIO6, 1);
	sleep(0.1);
	usb_send();
	sleep(0.1);
	
	ref_tx_wfm_load();
	
    int NoP=read_pulse_n();
	fft_tx();
    

    for(int loop_n=0; loop_n<NoP; loop_n++) {
      
		fprintf(stdout, "puse_no: %d \n", loop_n);

		gettimeofday(&myTime, NULL);    
		time_st = localtime(&myTime.tv_sec);    
		printf("Date : %d/%02d/%02d %02d:%02d:%02d.%06d\n",  
			time_st->tm_year+1900,     
			time_st->tm_mon+1,         
			time_st->tm_mday,          
			time_st->tm_hour,          
			time_st->tm_min,           
			time_st->tm_sec,           
			myTime.tv_usec            
			);

		pthread_create(&tid2, NULL, thread2, (void *) &mes_num);  // USB receive start
//		sleep(0.3); // wait
		pthread_join(tid2, NULL); // USB receive stop
		digitalWrite(GPIO25, 0);  

    
	      for(int y=0; y<mes_num; y++) {
			for (int x=0; x< FByte ;x++) {
				buf[y][x]          = buf1[y][x];
				buf[y][x+FByte]    = buf2[y][x];
				buf[y][x+FByte*2]  = buf3[y][x];
				buf[y][x+FByte*3]  = buf4[y][x];
			}

			make_rx("R", y, rxwfm, loop_n, time_st->tm_year+1900, time_st->tm_mon+1, time_st->tm_mday, time_st->tm_hour, time_st->tm_min, time_st->tm_sec, myTime.tv_usec);
			fft_rx(rxwfm, rxspc_pw);
			make_mxspc(mxspc_pw);
			ifft_mx();
			cal_env("R", crwv);
			int r_peak_n = extract_peaks("R",rightpeaks,leftpeaks);

			make_rx("L", y, rxwfm, loop_n, time_st->tm_year+1900, time_st->tm_mon+1, time_st->tm_mday, time_st->tm_hour, time_st->tm_min, time_st->tm_sec, myTime.tv_usec);
			fft_rx(rxwfm, rxspc_pw);
			make_mxspc(mxspc_pw);
			ifft_mx();
			cal_env("L", crwv);
			int l_peak_n = extract_peaks("L", rightpeaks,leftpeaks);
			
			/////// peaks_stdout /////
			fprintf(stdout,"peak_det_R: ");
			int n=0;
			for (int n=0; n<r_peak_n; n++){
				fprintf(stdout,"_%d", rightpeaks[n]);
			}
			fprintf(stdout,"\n");
			fprintf(stdout,"peak_det_L: ");
			n=0;
			for (int n=0; n<l_peak_n; n++){
				fprintf(stdout,"_%d", leftpeaks[n]);
			}
			fprintf(stdout, "\n\n");
			
			////// dist_direc_calc ////////
			int max_delay=290;
			int det_n=0;
			double LR_length=0.095;
			
			for (int n=0; n < r_peak_n; n++){
				for (int m=0; m < l_peak_n; m++){
					if(abs(rightpeaks[n]-leftpeaks[m]) <= max_delay){
						dist[det_n] = (rightpeaks[n]+leftpeaks[m])/2/2*340/1000;
						direc[det_n] = asin((rightpeaks[n]-leftpeaks[m])*340/(LR_length*1000000))*180/3.14;
						det_n++;
					}
				}		
			}
		
		
			/////// sending_data_log_writing //
			FILE *fp;
			if((fp = fopen("det_log.txt", "w"))==NULL) {
				printf("can't open send_message_file\n");
			} else {
				fprintf(stdout,"det_dist: ");
				for (int n=0; n < det_n; n++){
					fprintf(stdout,"_%d", dist[n]);
					fprintf(fp,"%d ", dist[n]);
					n++;
				}
				fprintf(stdout,"\n");
				fprintf(stdout,"det_deg: ");
				n=0;
				for (int n=0; n < det_n; n++){
					fprintf(stdout,"_%d", direc[n]);
					fprintf(fp,"%d ", direc[n]);
					n++;
				}
				
				fprintf(fp, "clock : %02d:%02d:%02d.%06d\n",     
				time_st->tm_hour,          
				time_st->tm_min,           
				time_st->tm_sec,           
				myTime.tv_usec            
				);
	
				fprintf(stdout, "\n\n");		  
				fprintf(fp, "\n");  
				fclose(fp);
			}
			
			// sending_flg_update /////////////////
			if((fp = fopen("flg.prm", "w"))==NULL) {
				printf("can't open send_message_file\n");
			} else {
				fprintf(fp,"%d ", loop_n);
				fclose(fp);
			}
		      //////////////////////////////
		      
		gettimeofday(&myTime, NULL);    
		time_et = localtime(&myTime.tv_sec);    
			printf("Date : %d/%02d/%02d %02d:%02d:%02d.%06d\n",   
			time_et->tm_year+1900,    
			time_et->tm_mon+1,         
			time_et->tm_mday,          
			time_et->tm_hour,          
			time_et->tm_min,           
			time_et->tm_sec,           
			myTime.tv_usec            
			);



	
			//////peak_detection_no_shokika////////////////////////
			for (int i=0; i<BOXNUM-1; i++){
				leftpeaks[i]=0;
				rightpeaks[i]=0;
				dist[i]=0;
				direc[i]=0;
			}		
		      //////////////////////////////

	      }	//_y_mes_num no for

	      fprintf(stdout, "\n\n\n\n");
 	}//loop_n no for
   post_thread2(); //USB serial close

}
