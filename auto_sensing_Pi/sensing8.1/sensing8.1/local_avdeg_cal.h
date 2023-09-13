//
//  loccal.h
//  kuruc
//
//  Created by 中出翔也 on 2019/04/18.
//  Copyright © 2019 shoya. All rights reserved.
//

#ifndef local_avdeg_cal_h
#define local_avdeg_cal_h

#include <stdio.h>
int avoidance_cal(double *pairdis,double *pairdeg,double *pair_alpha,int pairnum);
int local_cal(int leftsize,int rightsize,int *left,double *left_val,int *right,double *right_val,int tnum,int multipulseflag,int neckflag,int duration_short_ratio);

#endif /* ocal_avdeg_cal_h */
