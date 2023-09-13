import subprocess
import sys
import math
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import numpy as np
from udpdeg_main import senddeg as senddeg
from mapmaking import map_making as map_making

fasterflag = 2 # no show graph is 2 to faster program
multipulseflag = 1 #ここでマルチパルスセンシングを選ぶ１=nomal 2=double
neckflag = 2 #ダブルパルスセンシングのときのみ有効　sensing.cに引き渡すからここ変えればok。　1 avdeg   2直近物体に顔向ける　

duration_shortflag=1 # 2になったらshortをうつ。ずっと普通のモードにするには下のsuration_shortを１にして
next_duration_shortflag=1 #   移動距離は更新前の値だから保持しておく
duration_short_ratio = 1 #make short duration pulse ratio if num=5 ,make 1/5
duration_short_thr = 400 # [mm] if ther are no ditect object less than this[mm] , switch to emit short pulse
move_dis=200#こうもりが１回のパルス放射するときに動く距離
if duration_short_ratio==1:
    short_move_dis=move_dis
else:
    short_move_dis=int(2*move_dis/duration_short_ratio)#durationに比例して走行距離も短くする。
neckdeg = 0
x_now=0
y_now=0
dis_now=0
xlim = 2000
ylim = 2000
#subprocess.call('./hello')
plt.ion()

#subprocess.call(['cycfx2prog','prg:bench_in.ihx'])  #USBopen
subprocess.call(['rm','result_sensing/all_loccal.txt'])
subprocess.call(['rmmod','ftdi_sio'])
subprocess.call(['rmmod','usbserial'])
subprocess.call(['./ex_chirp','%d' %duration_short_ratio])  #configure frequency
#subprocess.call('./chkbox')   
#subprocess.call('./chkbox')
for i in range(51):   #MAX roops number
    print('\nsensing No%d' % (i+1))
    if duration_shortflag ==1:
        subprocess.call(['./sensing.bin','%d' %(i+1),'%d' %multipulseflag,'%d' %neckflag,'1'])
    else:
        print("------------------short pulse----------------------------")
        subprocess.call(['./sensing.bin','%d' %(i+1),'%d' %multipulseflag,'%d' %neckflag,'%d' %duration_short_ratio])
        
    
    x,y,pair_alpha,avdeg,mindis_deg,next_duration_shortflag = map_making.__init__(i+1,multipulseflag,duration_short_thr)#　ここで　shortにかわるので、危なかったら長いパルス放射でも直進はちいさくなる
    if duration_shortflag==1:
        plotavdegx = np.array([0,move_dis*math.cos(math.pi*avdeg/180)]) #プロット用の回避方向
        plotavdegy = np.array([0,move_dis*math.sin(math.pi*avdeg/180)]) #プロット用の回避方向
    else:
        plotavdegx = np.array([0,short_move_dis*math.cos(math.pi*avdeg/180)]) #プロット用の回避方向
        plotavdegy = np.array([0,short_move_dis*math.sin(math.pi*avdeg/180)]) #プロット用の回避方向
 #   plt.xlim([-1000,1000])
  #  plt.ylim([0,2000])

    if multipulseflag == 1 or (i+1)%2 == 1 :
        #print(pair_alpha)
        if(fasterflag==1):
            plt.clf()
            plt.xlim([-xlim,xlim])
            plt.ylim([0,ylim])
            plt.scatter(y, x, s=pair_alpha/100, c=pair_alpha, alpha=1, linewidths="2", edgecolors="red",marker="+",cmap=cm.Accent)
           # if duration_shortflag==1:
            #    plt.plot(duration_short_ratio*plotavdegy,duration_short_ratio*plotavdegx ,linewidth=5, color="pink")
            #else:
            plt.plot(plotavdegy,plotavdegx ,linewidth=5, color="pink")
            plt.colorbar()
            plt.draw()
            plt.pause(0.01)
  #      if multipulseflag == 2 :
        if multipulseflag == 2 :
            if neckflag == 1:
                neckdeg = avdeg
            elif neckflag == 2:
                neckdeg = mindis_deg
       #     senddeg.sendreceive(neckdeg+1000)# +1000 convert servo
        else: #singlesensingの時に動く
       #     senddeg.sendreceive(mindis_deg+1000)# シングルパルス時に確認のため直近物体に顔向ける。センシング的な意味はない。
       #     senddeg.sendreceive(0+1000)# 顔すぐ戻す
            print('走行開始')   
      #      senddeg.sendreceive(avdeg)
        #    if duration_shortflag==1:
        #        senddeg.sendreceive(-2000+move_dis)# -2000 convert strate motor
         #   else:
         #       senddeg.sendreceive(-2000+short_move_dis)
    else:#doublePS かつ偶数回めのとき
  #      print('入ったよー')
        if(fasterflag==1):
            plt.scatter(y, x, s=5, c="red", alpha=1, linewidths="2", edgecolors="red",marker="+")
            plt.plot(plotavdegy,plotavdegx ,linewidth=5, color="red")
            plt.draw()
            plt.pause(0.01)
        
        subprocess.call('./doubleps.bin')
        x,y,pair_alpha,avdeg,mindis_deg,plotavdegx,plotavdegy = map_making.__init__(i+1,0)
        if(fasterflag==1):
            plt.scatter(y, x, s=5, c="green", alpha=1, linewidths="2", edgecolors="green")
            plt.plot(plotavdegy,plotavdegx ,linewidth=10, color="blue")
            plt.draw()
            plt.pause(0.01)
        neckdeg = 0
      #  senddeg.sendreceive(neckdeg+1000)# +1000 convert servo
    #    senddeg.sendreceive(avdeg)
    
    x_now=x_now+plotavdegx[1]
    y_now=y_now+plotavdegy[1]
    dis_now=math.sqrt(pow(x_now,2)+pow(y_now,2))
    duration_shortflag=next_duration_shortflag
    print('carbat position= %d '%(y_now),'%d '%(x_now),' now distance= %d'%(dis_now))

 
print("finished!")


