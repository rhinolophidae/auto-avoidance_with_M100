#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Wed Apr 24 09:38:27 2019
all_loccal.txtを読み込んで、センシングごとの画像を作り、動画も作る。
2019.05.09   ver2 無駄なループを消して速度向上
2019.05.21   ver3 回転軸と送受信機の位置を考慮して定位物体を算出。ロボットのいちは回転軸の中心
2019.07.22   ver4 alphaの値を導入
2019.07.22   ver4 c\durationを変えて距離がk悪プログラムに対応。ただし real_car_disの定義をメインに合わせておくこと に注意
@author: shoya
"""
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import math
#import cv2
plt.ion()

textname = "result_sensing/all_loccal.txt"

car_running_dis = 200#一回のセンシングで動く距離mm ここは必ずメインと合わせておく
real_car_dis=0 #下に定義があるのでそこをメインと合わせる。!!!!!!!!!!!!!!!!!!  　特にノーマルモードのときは変更必要


car_to_neck=90#中心軸から顔までの距離
mapwidth  = 2000   #コースの横の広さ
maplength = 2000  #コースの縦の広さ
starty=0
startx=0
objfai=120 #ポールの直径

objx = []
objy = []

num = 0  # 定位数
tempnum = 0
i = 0   #ループで使う
trial = 0
trialnum = 0

dis = [0]
deg = [0]
x   =[]
y   =[]
pair_alpha = []
carposx =[startx]
carposy =[starty]
avdeg   =[0]
short_ratio=[1]
plotavdegx = [startx]
plotavdegy = [starty]

for line in open("result_sensing/objects.txt", "r"):
    data = line.split()
    objx.append(int(data[0]))
    objy.append(int(data[1])+objfai/2)


for line in open(textname, "r"):
        if line[0] != '$': 
            if line[0] == "#":#この文字が先頭にある行に定位数と回避角度を記載
                    trial = trial + 1                  
                    data = line.split() # 文字列を空白文字を区切りに分割
                    
                    avdeg.append(int(data[6]) + avdeg[trial-1] )#プロット用 座標系を基準とする絶対角度
                    short_ratio.append(int(data[9])) #プロット用　パルスの長さを作る
                    if short_ratio[trial-1]==5:
                        real_car_dis=2*car_running_dis/short_ratio[trial-1]
                    else :
                        real_car_dis=car_running_dis
                    carposx.append(real_car_dis*math.cos(math.pi*avdeg[trial]/180) + carposx[trial-1])#プロット用 ここの値は
                    carposy.append(real_car_dis*math.sin(math.pi*avdeg[trial]/180) + carposy[trial-1])
                                   
                    
            elif line == '\n': #空行のところでプロットをする。
                    plotavdegx.append(carposx[trial])#プロット用
                    plotavdegy.append(carposy[trial])#プロット用
                    pulsedirx = np.array([carposx[trial-1],carposx[trial-1]+10/(short_ratio[trial-1])*car_running_dis*math.cos(math.pi*avdeg[trial-1]/180)])
                    pulsediry = np.array([carposy[trial-1],carposy[trial-1]+10/(short_ratio[trial-1])*car_running_dis*math.sin(math.pi*avdeg[trial-1]/180)])
                    tempx   = np.array([0]*(i-tempnum))#プロット用
                    tempy   = np.array([0]*(i-tempnum))#プロット用
                    tempalpha = np.array([0]*(i-tempnum))
                    k=0
                    for k in range(i-tempnum):
                        tempx[k] = x[tempnum+k]
                        tempy[k] = y[tempnum+k]
                        tempalpha[k]=pair_alpha[tempnum+k]
                    plt.clf()  
                    plt.xlim([-1*mapwidth,mapwidth])
                    plt.ylim([0,maplength])
                    plt.plot(plotavdegy,plotavdegx ,alpha=0.3,linewidth=5, color="blue",label="trajectry",marker="o")
                    plt.plot(pulsediry,pulsedirx ,linewidth=2, color="green",label="pulse")
                    plt.scatter(objx, objy, s=100, c="black", alpha=1, linewidths="2", edgecolors="black",label="real_objects")
                    plt.scatter(tempy, tempx, s=tempalpha/8, c=tempalpha, alpha=1, linewidths="2", edgecolors="red",marker="+",label="objects",cmap=cm.Accent)
                    plt.legend()
                    plt.xlabel("Width[mm]")
                    plt.ylabel("Length[mm]")
                    plt.colorbar()
                    plt.draw()
                    plt.pause(0.1)
                    plt.savefig('result_sensing/figure/figure_%d.png' % trial)
                    #  car_running= normal_car_running_dis
                    tempnum=i

            else:
                    data = line.split()
                    i = i+1
                    
                    #      if int(data[0])<duration_short_thr:
                    #car_running_dis=int(2*normal_car_running_dis/duration_short_ratio)
                    
                    dis.append(int(data[0]))
                    deg.append(int(data[2]))
                    pair_alpha.append(int(data[6]))
                    x.append(dis[i] * math.cos(math.pi*(deg[i]+avdeg[trial-1])/180) + carposx[trial-1]+car_to_neck*math.cos(math.pi*(avdeg[trial-1])/180))
                    y.append(dis[i] * math.sin(math.pi*(deg[i]+avdeg[trial-1])/180) + carposy[trial-1]+car_to_neck*math.sin(math.pi*(avdeg[trial-1])/180))


                    
##全体を全て出力した、まとめの画像を出す#######
ar_pair_alpha=np.array(pair_alpha)


plt.clf()

plt.xlim([-1*mapwidth,mapwidth])
plt.ylim([0,maplength])
plt.scatter(y, x, s=ar_pair_alpha/8, c=ar_pair_alpha, alpha=1, linewidths="2", edgecolors="red",label="objects",marker="+",cmap=cm.Accent)
plt.colorbar()
plt.scatter(objx, objy, s=100, c="black", alpha=0.3, linewidths="2", edgecolors="black",label="real_objects")
plt.plot(carposy,carposx ,linewidth=5, color="blue",label="trajectry",marker="o",alpha=0.3)

plt.legend()
plt.xlabel("Width[mm]")
plt.ylabel("Length[mm]")

plt.draw()
plt.pause(2)
plt.savefig('result_sensing/figure/allfigure_%d.png' % trial)
                    
############# 以下は動画作成用  ###############                  
fourcc = cv2.VideoWriter_fourcc('m','p','4','v')
video = cv2.VideoWriter('video.mp4', fourcc, 1.5, (640, 480))

for i in range(1, trial+1):
    img = cv2.imread('figure/figure_%d.png' % i)
    img = cv2.resize(img, (640,480))
    video.write(img)
img = cv2.imread('figure/allfigure_%d.png' % trial)
img = cv2.resize(img, (640,480))
video.write(img)

video.release()
