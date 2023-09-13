# -*- coding: utf-8 -*-
"""
Created on Thu Aug 19 13:46:12 2021

@author: yasuf
"""

import numpy as np
from scipy.fftpack import fft, ifft
from scipy.signal import hilbert
import scipy.signal as signal
from multiprocessing import Pool
import pathlib
import glob
import matplotlib.pyplot as plt
import os

def draw_wv(file, data_name, x_data, y_data, x_min, x_max, y_min, y_max, x_asp, y_asp):
    x_data=x_data*1000
    fig, (ax1) = plt.subplots(1,1, figsize=(x_asp, y_asp), sharex=True, constrained_layout=True,fontsize=18)
    ax1.plot(x_data, y_data)    
    
    ax1.set_xlim(x_min*1000, x_max*1000)
    ax1.set_ylim(y_min, y_max)
 #   ax1.set_title("$f_0$ = {} [kHz]  $f_1$ = {} [kHz]  T = {} [msec]".format(*(str(val.parent.parent).split('/')[-1].split('_'))))
    ax1.set_ylabel("waveform")
    ax1.set_xlabel("Time [msec]")
    fig.savefig("analysis_figures/{}_{}_{}_{}_{}_{}.png".format(data_name, *(file.split('_'))))
    
    plt.pause(0.5)
    plt.close()

def fft_tx(val):
    fname = val / 'chk_txwv.dat'
    
    with open(fname) as f:
        txwv = [float(s.strip()) for s in f.readlines()]
    print("len(txwv)", len(txwv))
    txspc = fft(txwv)    
    fname = val / 'chk_txspc.dat'
    with open(fname, mode='w') as f:
        f.write('\n'.join(map(str, txspc)))
    return txspc

def read_txspc(val):
    fname = val.parent.parent / "chk_txspc.dat"
    with open(fname) as f:
        txspc = [complex(s.strip()) for s in f.readlines()]
    return txspc

def read_rxwv_with_clock(val):
#    with open(val.parent/"chk_rxwv_left_0.dat") as f:
    with open(val) as f:
        rxwv = [s.strip() for s in f.readlines()]
        time = rxwv[0]
        rxwv = [float(s) for s in rxwv[1::]]
    rxwv_mean=np.mean(rxwv)
    rxwv = [s-rxwv_mean for s in rxwv]
    return time, rxwv 

def X_cor_env(rxwv, txspc):
    crwv = ifft(fft(rxwv) * np.conj(txspc)).real
    hlbwv = hilbert(crwv)
    env = np.abs(hlbwv)  
    return hlbwv, env
    
def draw_wv_Xcor_env(data):#data[0]:val, data[1]:sensing_time, data[2]:rxwv, data[3]:env, data[4]:env
    val = data[0]
    time = data[1]
    rxwv = data[2]
    hlbwv = data[3]
    env = data[4]
    t0 = range(0, len(rxwv))
    t =[i / 1000 for i in range(0, len(rxwv))]
    
    fig, (ax1, ax2) = plt.subplots(2,1, sharex=True, constrained_layout=True)    

    ax2.plot(t, env)
    ax1.plot(t, rxwv)
    ax2.set_xlim(0,max(t))
    ax2.set_ylim(0,500000)
    ax1.set_ylim(-800, 800)
    i = int(str(val).split('_')[-1][:-4])
#    i = str(val).split('_')[-1][:-4]
    side = str(val).split('_')[-2]
    ax1.set_title("$f_0$ = {} [kHz]  $f_1$ = {} [kHz]  T = {} [msec]".format(*(str(val.parent.parent).split('/')[-1].split('_'))))#LINUX
#    ax1.set_title("$f_0$={}[kHz]  $f_1$={}[kHz]  T={}[msec]  k1={} k2={}".format(*(str(val).split('\\')[1].split('_'))))#WIN
    
    ax1.text(0,650,"Time: {} ".format(time))
    ax2.set_xlabel("Time [msec]")
    ax2.set_ylabel("Correlation")
    ax1.set_ylabel("Recieved Waveform")
    print(val)
    fig.savefig(val.parent.parent/"graph/crwv_{}_{}_.png".format(side, i))
#    plt.show(fig)
    plt.pause(0.2)
    plt.close(fig)
    
 
    fname = val.parent.parent/'hilbert/chk_hilbert_{}_{}.dat'.format(side, i)
    with open(fname, mode='w') as f:
        f.write('\n'.join(map(str, hlbwv)))

    fname = val.parent.parent/'envelope/chk_env_{}_{}.dat'.format(side, i)
    with open(fname, mode='w') as f:
        f.write('\n'.join(map(str, env)))
    

def draw_spectrogram(file, data_name, x_data, y_data, z_color, cmax):
    x_data=x_data*1000
    y_max=100000
    y_min=40000
    plt.figure(figsize=(6.0, 6.0))
    plt.pcolormesh(x_data, y_data, z_color, cmap="Greys")
    plt.xlim([min(x_data),max(x_data)])
    plt.ylim([y_min,y_max])
#    plt.clim([0,0.3])
    plt.clim([0,cmax])
    plt.xlabel("Time [sec]")
    plt.ylabel("Freq [Hz]")
    plt.colorbar()
    print("file:", file)
    plt.savefig("analysis_figures/{}_{}_{}_{}_{}.png".format(data_name, *(file.name.split('_'))))
 #   plt.savefig("analysis_figures/{}_{}_{}_{}_{}.png".format(data_name, *(file.split('_'))))
    plt.pause(0.5)
    plt.close()

def origin_draw_func(val):
    txspc = fft_tx(val.parent.parent)
    time, rxwv = read_rxwv_with_clock (val)
    hlbwv, env = X_cor_env(rxwv, txspc) 
    draw_wv_Xcor_env([val, time, rxwv, hlbwv, env])#data[0]:val, data[1]:sensing_time, data[2]:rxwv, data[3]:env
    print("rxwv", len(rxwv),type(rxwv))
    
    spc_f_tx, spc_t_tx, spc_pw_tx = signal.spectrogram(np.array(rxwv), fs=10**6, nperseg=4096)
    draw_spectrogram(val, "tx_spctrgrm", spc_t_tx, spc_f_tx, spc_pw_tx, 0.5)
    
def main():
    p = Pool()
    dataDir = pathlib.Path('result_sensing/')
    print(os.listdir(dataDir))
    params = [dataDir / prm for prm in os.listdir(dataDir)]
#    p.map(fft_tx, params)    #ikki ni spc tx wo yomikomu baai ha kono kansuu wo shiyou
    
#    with open("condition.prm") as f:
#        params = [dataDir / s.strip() for s in f.readlines()]
    print("params:", params)
    print()    

    for prm in params:
        p.map(fft_tx, [prm])
        
        (prm/'envelope').mkdir(exist_ok=True)
        (prm/'hilbert').mkdir(exist_ok=True)
        (prm/'graph').mkdir(exist_ok=True)
        
        files = list(prm.glob('rxwv/chk_rxwv_right*'))
        print("files",files)
        p.map(origin_draw_func, files)

        files = list(prm.glob('rxwv/chk_rxwv_left*'))
        print("files",files)
        p.map(origin_draw_func, files)
        
if __name__ == '__main__':
    main()

 #       spc_f_tx, spc_t_tx, spc_pw_tx = signal.spectrogram(rxwv, fs=10**6, nperseg=512)
 #    spectrogram_culc_draw(val, time, rxwv)
