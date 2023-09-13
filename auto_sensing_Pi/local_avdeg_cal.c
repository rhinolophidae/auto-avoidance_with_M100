//
//  loccal.c
// 
//
//  Created by 中出翔也 on 2019/04/18.
//  Copyright © 2019 shoya. All rights reserved.
// local_cal呼び出すと勝手にavoidance_calも行う
//  20190517 回避方向算出式を変えたよ。　by NKD
// 2019 0709 mice length =100 に変更
// 2019 0710 定位方向算出式のミスを改善　
// 2019 1015 細かい精度で距離と角度を算出


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "local_avdeg_cal.h"

#define micelength 100
#define velocity 340
#define Pi 3.14
#define fai 60            //ポールの半径 mm
//#define alpha  400.0 //斥力の強さを決めるパラメーター おそらく意味なし

int avoidance_cal(double *pairdis,double *pairdeg,double *pair_alpha,int pairnum)
{
    double Sum_vector_x = 0.0;
    double Sum_vector_y = 0.0;
    double vector_length = 0.0;
    double distance_force = 0.0;
    double direction_force = 0.0;
 
    double avoidancedeg = 0.0;
    
    for (int i=0; i < pairnum ; i++)
    {
     //   distance_force = sqrt(alpha / pairdis[i]); //
        distance_force = sqrt(1.0 / pairdis[i]); 
        direction_force = sin(atan(fai / pairdis[i]));
        vector_length = pair_alpha[i]*distance_force * direction_force ; //検知物体固有のalphaを導入
        Sum_vector_x =  Sum_vector_x + vector_length * sin( pairdeg[i] / 180.0 * 3.14);//正負の向きは反映されて
        Sum_vector_y =  Sum_vector_y + vector_length * cos( pairdeg[i] / 180.0 * 3.14);
    }
    //avoidancedeg = (180.0 / Pi ) * atan(-Sum_vector_x / (1 - Sum_vector_y ) );
    if (Sum_vector_y != 0){             //分母が0となって発散するため、それを制御。そのときはavdeg=0のまま
        avoidancedeg = (180.0 / Pi ) * atan(-Sum_vector_x / (Sum_vector_y ) );
    }
   // printf("%f %f %f\n",Sum_vector_x,Sum_vector_y,avoidancedeg);
    return (int)avoidancedeg;
}


int local_cal(int leftsize,int rightsize,int *left,double *left_val,int *right,double *right_val,int tnum,int multipulseflag,int neckflag,int duration_short_ratio)
{
    int maxpairnum =  1024;     //検知できる物体の最大数
    double pairdis[maxpairnum];//物体の距離box
    double pairdeg[maxpairnum];//物体の角度box
    double pair_alpha[maxpairnum];//物体の確かさ;
    int pairnum = 0,pairnumtmp=0,avdegtmp=0,mindis_degtmp=0;                                       //実際のbox数
    int maxdelt = (int)(1.0*micelength*1000/velocity);      //90度方向から音が来た場合の左右間時間差[us]
   // printf("maxdelt=%d [us]\n",maxdelt);
    double disl=0,disr=0,alpha=0;
    double dltdis =0.0;
    FILE *fp;

/////////////////////ダブルパルスセンシング2回目の時はneckdeg修正のためにtmpを読み込む/////////////////
    if (tnum%2 == 0 &&  multipulseflag == 2 ){
        fp = fopen("result_sensing/loccal_tmp.txt", "r");
        if( fp == NULL ) {
            perror("ファイルの読み込みに失敗！\n");
            return 1;
        }
        fscanf(fp, "%*s %*s %*s %d %*s %*s %d",&pairnumtmp,&avdegtmp);
        int distemp,degtemp,alphatemp,disbox[pairnumtmp],degbox[pairnumtmp],alphabox[pairnum],mindis =10000,minflag=0;
        int i=0;
        
        //  printf("num=%d avoidancedeg=%d\n", pairnum,avdeg);
        
        while(fscanf(fp, "%d %*s %d %*s %*s %*s %d",&distemp, &degtemp,&alphatemp) != EOF) {
            disbox[i] = distemp;
            degbox[i] = degtemp;
            alphabox[i]= alphatemp;
            if (disbox[i] < mindis){ //ここをalphaの最大値にしてもいいかも
                mindis = disbox[i];
                mindis_degtmp=degbox[i];
                minflag = i;
            }
            //       printf("disx=%d disy=%d\n", disbox_x[i],disbox_y[i]);
            i++;
        }
        fclose(fp);
    }
/////////////////////tmpを読み込む終了//////////////////////////////////////////////////
///////////////////////////ここからペアを作る////////////////////////////////////////////
    for (int k=0; k<rightsize;k++){
        for (int l=0; l<leftsize;l++){
            if (right[k] -left[l] > (-1.0)*maxdelt && right[k] -left[l] < maxdelt  ){
     //           printf("%d > %lf\n",right[k] -left[l], (-1.0)*maxdelt);
                disr = right[k]*velocity/(1000.);          //右耳への到達距離[mm]
                disl = left[l]*velocity/(1000.);
                dltdis = disl - disr;                       //左-右の到達距離[mm]
              
                pairdis[pairnum] = (disr + disl) / 4.;                       //左右による平均距離*往復距離
                pairdeg[pairnum] = (180.00 /Pi ) * asin(dltdis / micelength);  //右側をプラスとした検知角度
                pair_alpha[pairnum]=right_val[k] * left_val[l] *1000;                      //２つのvalをかけることで、その検知物体固有の確かさ（危険度）をつくる.*1000はpythonで小数値を扱いづらいため
    //            printf("disr = %lf disl = %lf\n",disr,disl);
                 if (tnum%2 == 0 &&  multipulseflag == 2 ){ ///ここでダブルパルスセンシング2回目の時に角度補正
                     if(neckflag==1){ //avdegが回避角度
                         pairdeg[pairnum] =pairdeg[pairnum] + avdegtmp;
                     }
                     else{            //直近物体が回避角度
                         pairdeg[pairnum] =pairdeg[pairnum] + mindis_degtmp;
                     }
                 }
                pairnum++;
            }
        }
    }
    printf("# pairnum = %d avoidancedeg = %d \n", pairnum,avoidance_cal(pairdis, pairdeg,pair_alpha, pairnum));
    for (int k=0; k<pairnum;k++){
        printf("%7.4f mm %7.4f deg :alpha = %d\n",pairdis[k], pairdeg[k],(int)pair_alpha[k]);
    };
////////////////////////////ペア作り終了/////////////////////////////////////////////////
    
    //ここから,ファイル出力　temp
    FILE *outputfile;         // 出力ストリーム
    if (tnum%2 == 0 &&  multipulseflag == 2 ){
        outputfile = fopen("result_sensing/loccal_tmp2.txt", "w");  // doublepuls
    }
    else{
        outputfile = fopen("result_sensing/loccal_tmp.txt", "w");  // ファイルを書き込み用にオープン(開く)
    }
        if (outputfile == NULL) {          // オープンに失敗した場合
        printf("cannot open\n");         // エラーメッセージを出して
        exit(1);                         // 異常終了
    }
    
    fprintf(outputfile,"# pairnum = %d avoidancedeg = %d pulse_duration_ratio =  %d \n", pairnum,avoidance_cal(pairdis, pairdeg, pair_alpha,pairnum),duration_short_ratio);
    for (int k=0; k<pairnum;k++){
           fprintf(outputfile, "%7.4f mm %7.4f deg :alpha = %d\n",pairdis[k], pairdeg[k],(int)pair_alpha[k]); // ファイルに書く
    };
    fclose(outputfile);          // ファイルをクローズ(閉じる)
    
    //ここから,ファイル出力　全部保存
    outputfile = fopen("result_sensing/all_loccal.txt", "a");  // ファイルを書き込み用にオープン(開く)
    
    if (outputfile == NULL) {          // オープンに失敗した場合
        printf("cannot open\n");         // エラーメッセージを出して
        exit(1);                         // 異常終了
    }
    fprintf(outputfile,"$ trial = %d\n# pairnum = %d avoidancedeg = %d pulse_duration_ratio =  %d \n",tnum, pairnum,avoidance_cal(pairdis, pairdeg,pair_alpha, pairnum),duration_short_ratio);
    for (int k=0; k<pairnum;k++){
        fprintf(outputfile, "%7.4f mm %7.4f deg :alpha = %d\n",pairdis[k], pairdeg[k],(int)pair_alpha[k]); // ファイルに書く
    };
    fprintf(outputfile,"\n");
    fclose(outputfile);          // ファイルをクローズ(閉じる)
    return 0;
}






