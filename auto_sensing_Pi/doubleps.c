//
//  main4.c
//  kuruc
//
//  Created by 中出翔也 on 2019/04/15.
//  Copyright © 2019 shoya. All rights reserved.
//　0711 alphaをまだ導入できていない
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "local_avdeg_cal.h"

#define detectrange 20.0 //２回のパルスで半径がこの距離以内にないものは消す 2*faiと同じくらい？

int distemp, degtemp,alphatemp;
int pairnum,pairnum2,pairnumdps=0, avdeg , avdeg2,avdegdps;
FILE *fp;

int main(int argc, char *argv[])
{

////////////ファイル読み込み開始///////////////
    /* ファイルオープン */
    fp = fopen("result_sensing/loccal_tmp.txt", "r");
    /* ファイルが適切に読み込まれているかを確認 */
    if( fp == NULL ) {
        perror("ファイルの読み込みに失敗！\n");
        return 1;
    }
    /* テキストの読み込み&出力 */
    fscanf(fp, "%*s %*s %*s %d %*s %*s %d",&pairnum,&avdeg);
    int disbox[pairnum],degbox[pairnum],disbox_x[pairnum],disbox_y[pairnum];
    double alphabox[pairnum];
    int i=0;
    
    while(fscanf(fp, "%d %*s %d %*s %*s %*s %d",&distemp, &degtemp,&alphatemp) != EOF) {
        disbox[i] = distemp;
        degbox[i] = degtemp;
        alphabox[i]= alphatemp;
        disbox_x[i] = disbox[i] * sin(( degbox[i] / 180.0 * 3.14));
        disbox_y[i] = disbox[i] * cos(( degbox[i] / 180.0 * 3.14));
        i++;
    }
    fclose(fp);
//////////////tmp読み込む終了////////////////////
//////////////tmp2読み込み開始//////////////////
    fp = fopen("result_sensing/loccal_tmp2.txt", "r");
    /* ファイルが適切に読み込まれているかを確認 */
    if( fp == NULL ) {
        perror("ファイルの読み込みに失敗！\n");
        return 1;
    }
    /* テキストの読み込み&出力 */
    fscanf(fp, "%*s %*s %*s %d %*s %*s %d",&pairnum2,&avdeg2);
    int disbox2[pairnum2],degbox2[pairnum2],disbox_x2[pairnum2],disbox_y2[pairnum2];
    double alphabox2[pairnum];
    int k=0;
    
    while(fscanf(fp, "%d %*s %d %*s %*s %*s %d",&distemp, &degtemp,&alphatemp) != EOF) {
        disbox2[k] = distemp;
        degbox2[k] = degtemp;
        alphabox2[i]= alphatemp;
        disbox_x2[k] = disbox2[k] * sin(( degbox2[k] / 180.0 * 3.14));
        disbox_y2[k] = disbox2[k] * cos(( degbox2[k] / 180.0 * 3.14));
        k++;
    }
    fclose(fp);
   
//////ここまでは二つのファイル読み込み////////////////////
//////ここからがdoublepulseセンシングの統合///////// 
//２回のセンシングの照らし合わせをおこい、ゴーストを消す
    int maxpairnum = pairnum +pairnum2;
    double disboxdps[maxpairnum],degboxdps[maxpairnum],alphaboxdps[maxpairnum];
    
    for (i=0; i<pairnum;i++){
        for (k=0; k<pairnum2;k++){
            if(pow(disbox_x[i]-disbox_x2[k], 2.0) + pow(disbox_y[i]-disbox_y2[k], 2.0) < pow(detectrange,2.0) ){
                disboxdps[pairnumdps] = disbox[i];
                degboxdps[pairnumdps] = degbox[i];
                alphaboxdps[pairnumdps] = (alphabox[i] + alphabox2[k])/2 ; //平均とったがこれでいいのか検討するべし
                pairnumdps++;
                break;
            }
        }
    }
    
    for (k=0; k<pairnum2;k++){
        for (i=0; i<pairnum;i++){
            if(pow(disbox_x[i]-disbox_x2[k], 2.0) + pow(disbox_y[i]-disbox_y2[k], 2.0) < pow(detectrange,2.0) ){
                disboxdps[pairnumdps] = disbox2[k];
                degboxdps[pairnumdps] = degbox2[k];
                alphaboxdps[pairnumdps] = (alphabox[i] + alphabox2[k])/2 ; //平均とったがこれでいいのか検討するべし
                pairnumdps++;
                break;
            }
        }
    }
///////ここまで////////////////////////////////
//////////ここから,ファイル出力　temp_dps       // 出力ストリーム
    fp = fopen("result_sensing/loccal_tmp_dps.txt", "w");  // ファイルを書き込み用にオープン(開く)
    fprintf(fp,"# pairnum = %d avoidancedeg = %d \n", pairnumdps,avoidance_cal(disboxdps,degboxdps,alphaboxdps ,pairnumdps)); 
    if (fp == NULL) {          // オープンに失敗した場合
        printf("cannot open\n");         // エラーメッセージを出して
        exit(1);                         // 異常終了
    }
    
    for (int k=0; k<pairnumdps;k++){
        fprintf(fp, "%d mm %d deg :alpha = %d\n", (int)disboxdps[k], (int)degboxdps[k],(int)alphaboxdps[k]); // ファイルに書く
    };
    fclose(fp);          // ファイルをクローズ(閉じる)
    
    //ここから,ファイル出力　全部保存
    fp = fopen("result_sensing/all_loccal.txt", "a");  // ファイルを書き込み用にオープン(開く)
    fprintf(fp,"& DPSpairnum = %d DPSavoidancedeg = %d \n", pairnumdps,avoidance_cal(disboxdps,degboxdps,alphaboxdps, pairnumdps));//*ここも
    if (fp == NULL) {          // オープンに失敗した場合
        printf("cannot open\n");         // エラーメッセージを出して
        exit(1);                         // 異常終了
    }
    
    printf("& DPSpairnum = %d DPSavoidancedeg = %d \n", pairnumdps,avoidance_cal(disboxdps,degboxdps,alphaboxdps, pairnumdps));//ここも
    for (int k=0; k<pairnumdps;k++){
        fprintf(fp, "%d mm %d deg :alpha = %d\n", (int)disboxdps[k], (int)degboxdps[k],(int)alphaboxdps[k]); // ファイルに書く
     //   printf("%d mm %d deg \n", (int)disboxdps[k], (int)degboxdps[k]);
    };
    fprintf(fp,"\n");
    fclose(fp);          // ファイルをクローズ(閉じる)
    
    return 0;

}
