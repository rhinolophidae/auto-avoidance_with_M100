#!/usr/bin/env python3

import numpy as np
import matplotlib.pyplot as plt
import math

class map_making():
    def __init__(trial,multipulseflag,duration_short_thr):

        num = 0  # 定位数
        avdeg = 0  #回避角度
        i = 0   #ループで使う
        duration_shortflag=1 #1 : nomal 2:switch short pulse
        mindis = 10000
        mindis_deg = 0
        if multipulseflag == 0 :
            txtname = "result_sensing/loccal_tmp_dps.txt"
        elif multipulseflag == 1 or trial%2 == 1 :
            txtname = "result_sensing/loccal_tmp.txt"
        else:
            txtname = "result_sensing/loccal_tmp2.txt"
            
        for line in open(txtname, "r"):
            if line[0] == "#":#この文字が先頭にある行に定位数と回避角度を記載
                data = line.split() # 文字列を空白文字を区切りに分割
                num = int(data[3]) # 定位数を取り出す
                avdeg = int(data[6]) # 回避角度を取り出す
                dis = np.array([0]*num)
                deg = np.array([0]*num)
                pair_alpha = np.array([0]*num)
                x   = np.array([0]*num)#プロット用
                y   = np.array([0]*num)#プロット用
             #   plotavdegx = np.array([0,200*math.cos(math.pi*avdeg/180)]) #プロット用の回避方向
             #   plotavdegy = np.array([0,200*math.sin(math.pi*avdeg/180)]) #プロット用の回避方向
                
            else:  
                data = line.split() 
                dis[i] = float(data[0]) 
                deg[i] = float(data[2])
                pair_alpha[i] = int(data[6])
          #      print(pair_alpha[i])
                x[i] = dis[i] * math.cos(math.pi*deg[i]/180)
                y[i] = dis[i] * math.sin(math.pi*deg[i]/180)
                if dis[i]< mindis:
                    mindis = dis[i]
                    mindis_deg =deg[i]
                    
                if dis[i]<duration_short_thr:
                    duration_shortflag =2
                i = i+1
 
      #  return x,y,pair_alpha,avdeg,mindis_deg,plotavdegx,plotavdegy,duration_shortflag
        return x,y,pair_alpha,avdeg,mindis_deg,duration_shortflag
