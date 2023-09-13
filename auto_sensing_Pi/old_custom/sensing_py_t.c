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

#define BOXNUM  32768  //  data points for calculating array
#define DataNum 32768
#define FByte DataNum*3/4 	//1回のFT_Read命令によって取得するデータ数
#define MAX_MEAS_NUM 600
#define TRIGTSD	960
#define FS 1.e6        //  sampling frequency
#define BUFLEN 256

#define TIMEOUT (5*1000)
#define EP6	0x86

#define direct_pulse_time 500 //直龍波us 実際は 50000/340 = 147 us
#define failmax  3 //1default は10 
#define peaksmax  50 //
#define thd_limit  0.2 //相関波形のMAX値に対する割合.閾値になる defalt は0.2

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

unsigned char buf1[MAX_MEAS_NUM][FByte];//USBデータ取得用バッファとして使う
unsigned char buf2[MAX_MEAS_NUM][FByte];//USBデータ取得用バッファとして使う
unsigned char buf3[MAX_MEAS_NUM][FByte];//USBデータ取得用バッファとして使う
unsigned char buf4[MAX_MEAS_NUM][FByte];//USBデータ取得用バッファとして使う
unsigned char buf[MAX_MEAS_NUM][FByte*4];

// FTDI IC変数設定
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
int f0, f1, T, count;


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

	sprintf(filename, "result_sensing/%d_%d_%d/chk_txwv.dat", f0, f1, T);
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

void make_rx(char* receiver_side, int mes_index)//生波形データを変数に取り込む(TRIGTSDに作用されなくした)。
{
	signed short sb[DataNum];
	int j,i,k;
	FILE *fp,*fpcopy;
	char filename[256];
		
		if (receiver_side == "L"){
			sprintf(filename, "result_sensing/%d_%d_%d_%d/rxwv/chk_rxwv_left_%d.dat", f0, f1, T, count, mes_index);
			fp =fopen(filename, "w");
		}
		else{
			sprintf(filename, "result_sensing/%d_%d_%d_%d/rxwv/chk_rxwv_right_%d.dat", f0, f1, T, count, mes_index);
			fp =fopen(filename, "w");
		}
		
		if (NULL ==fp){
		fprintf(stderr, "fileopen error!6");	 exit(7);
		}
		setvbuf(fp, NULL, _IOFBF, 512*1024);
		j = 0;
		i = 0;
		
		for(k = 0;k < DataNum;k++)
			{	
				if (receiver_side == "L"){//*ここが左耳の条件　ここをかえよう
				sb[j]  = (buf[mes_index][3*k]&0x00ff)|((buf[mes_index][3*k+1]<<8)&0x0300);	// ch1
				j++;
				}
				else{//*ここが右耳の条件
				sb[j] = ((buf[mes_index][3*k+1]>>2)&0x003f) |  ((buf[mes_index][3*k+2]<<6)&0x03C0); // ch2 
				j++;
				}
					
				rxwfm[i] = (double)(sb[j-1] - 0x0200);
				fprintf(fp, "%f\n", rxwfm[i]);
				i++;
				if ( i >= adc_rec_num )		break;
			}
		fclose(fp);
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


void param_read_fpga(void ) // FPGA内部パラメーター読み込み
	{
	FILE *fp;
	char buf[BUFLEN];

	if( NULL == (fp = fopen("sensing_fpga.prm","r")) )
		{	fprintf(stderr, "Cannot open data file\n");	exit(0);	}
		    fgets(buf, BUFLEN, fp);	adc_rec_setting = atoi(buf);
        fgets(buf, BUFLEN, fp);	mes_tim_setting = atoi(buf);
        fgets(buf, BUFLEN, fp);	mes_num_setting = atoi(buf);

	fclose(fp);
	}

void gpio_init(void) // GPIOの初期化
{
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

int table_fpga_setting_1(int adc_rec_setting) // FPGAパラメーター変換テーブル(ADC受信回数)

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

int table_fpga_setting_2(int mes_num_setting) // FPGAパラメーター変換テーブル(測定回数)

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

int main(int argc, char *argv[]){

    //変数の宣言//
	pthread_t tid1, tid2;
    int gpio_setting[5];
	
	f0 = atoi(argv[1]);
	f1 = atoi(argv[2]);
	T = atoi(argv[3]);
	count = atoi(argv[4]);
	

	//FPGA制御パラメータの読み込み//
    param_read_fpga();
    //パラメーターデジタルデータから実際値への変換
	adc_rec_num = table_fpga_setting_1(adc_rec_setting);
	mes_num     = table_fpga_setting_2(mes_num_setting);

    //各GPIOの初期化
    gpio_init();
    //FPGAに対するリセット印可
	digitalWrite(GPIO8, 0);
	sleep(0.1);
	digitalWrite(GPIO8, 1); // リセット解除
    sleep(0.1);

    //設定パラメータのFPGAへの反映(ADC受信回数)
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

    //設定パラメータのFPGAへの反映(測定時間)
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

    //設定パラメータのFPGAへの反映(測定回数)
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

    //駆動パルスデータの読み込み//
    tx_wfm_load(f0, f1, T, count);
    //駆動パルスデータのFPGAへの反映
	usb_send();
    sleep(0.1);

    //補助パルスデータの読み込み//
	pulse_aux_load();
	//補助パルスデータのFPGAへの反映
	digitalWrite(GPIO6, 1);
	sleep(0.1);
	usb_send();
	sleep(0.1);	

    ////////////////////ここまでがFPGA initial処理

		pthread_create(&tid2, NULL, thread2, (void *) &mes_num);  // USB receive start
		sleep(1); // wait
		pthread_join(tid2, NULL); // USB receive stop
		digitalWrite(GPIO25, 0);  
		post_thread2();

		for(int y=0; y<mes_num; y++) {
		  for (int x=0; x< FByte ;x++) {
		    buf[y][x]          = buf1[y][x];
		    buf[y][x+FByte]    = buf2[y][x];
		    buf[y][x+FByte*2]  = buf3[y][x];
		    buf[y][x+FByte*3]  = buf4[y][x];
		  }
		}
    
		for(int mes_index=0; mes_index<mes_num; mes_index++) {
      		make_rx("R", mes_index);
			make_rx("L", mes_index);
		}
}
