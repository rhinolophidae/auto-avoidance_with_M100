#!/usr/bin/env python
# -*- coding: utf-8 -*-

import numpy as np
from scipy.fftpack import fft, ifft
from scipy.signal import hilbert, argrelmax
from multiprocessing import Pool
import pathlib
import glob
import pypysocket_host_real_v1 as ppsh
import time

import os
import sys
import fcntl
import re

def fft_tx(val):
    fname = val / 'chk_txwv.dat'
    with open(fname) as f:
        txwv = [s for s in map(float,f.readlines())]
    txspc = fft(txwv)
        
    return txspc


def rx_read_r(val):
    with open(val) as f:
        rxwv = [s for s in map(float,f.readlines())]
    ################### DC_cut ############################
    DC_correc = np.full(len(rxwv),14.0)
    rxwv =np.array(rxwv)
    rxwv=rxwv-DC_correc 
    ################### DC_cut ############################
    return rxwv

def rx_read_l(val):
    with open(val) as f:
        rxwv = [s for s in map(float,f.readlines())]
    return rxwv
        
def fft_rx(rxwv):
    rxspc=fft(rxwv)
    return rxspc

def func(txspc_get,rxspc_get):
    txspc=txspc_get
    rxspc=rxspc_get
    crwv = ifft(rxspc * np.conj(txspc)).real
    hlbwv = hilbert(crwv)
    env = np.abs(hlbwv)  

    return env


def pk_det_func(inputs):
    env_thres=50000
    t_det_min=500
    t_det_max=32768
    pk_union_t=200
        
    env_over_thres_t =[i for i, x in enumerate(inputs) if x >= env_thres and t_det_min < i and i< t_det_max] #ikichi goe no id to amp wo get    
    env_pk_t=[t for t in env_over_thres_t if (inputs[t+1]-inputs[t]) * (inputs[t]-inputs[t-1])<0.0]
    env_pk_x= [x for x in inputs[env_pk_t]]
    
#    print("before")        
#    print("env_pk_t")        
#    print(env_pk_t)        
#    print("env_pk_x")        
#    print(env_pk_x)        
#    print()        
    
    i=0
    while i< len(env_pk_t)-1:        
        while env_pk_t[i+1]- env_pk_t[i]< pk_union_t:            
            if env_pk_x[i] < env_pk_x[i+1]:
                env_pk_x.pop(i)
                env_pk_t.pop(i)
            else:
                env_pk_x.pop(i+1)
                env_pk_t.pop(i+1)
            if i>= len(env_pk_t)-1:
                break
        i=i+1
        
#    print("after")        
#    print("env_pk_t")        
#    print(env_pk_t)        
#    print("env_pk_x")        
#    print(env_pk_x)
#    print()
        
    return env_pk_t


def pkdet_func_neo(inputs):    
    env_thres=50000
    t_det_min=500
    pk_union_t=200
    
    #new_calc
    peaks_t=np.array(argrelmax(inputs))[0]
    echo_t=peaks_t[inputs[peaks_t]>env_thres]
    echo_t=echo_t[echo_t>t_det_min]
    
    echo_t=echo_t.tolist()
    echo_pw= [x for x in inputs[echo_t]]
    
#    print("echo_t")
#    print(type(echo_t))
#    print(echo_t)
#    print("echo_pw")
#    print(type(echo_pw))
#    print(echo_pw)
    
    i=0
    while i< len(echo_t)-1:        
        while echo_t[i+1]- echo_t[i]< pk_union_t:            
            if echo_pw[i] < echo_pw[i+1]:
                echo_pw.pop(i)
                echo_t.pop(i)
            else:
                echo_pw.pop(i+1)
                echo_t.pop(i+1)
            if i>= len(echo_t)-1:
                break
        i=i+1
        
    return echo_t


def dist_direc_calc(val_rt,val_lt):
#    print(val_rt)
#    print(val_lt)
    c=340.0 #[m/s]
    lr_length=0.1 #[m]
    max_t=lr_length/c*1000000 #[us]

    det_dist=[c*(r+l)/4000000 for r,l in zip(val_rt, val_lt) if abs(r-l) <= max_t]
    det_deg=[np.arcsin(c*((r-l)/1000000)/lr_length)/np.pi*180 for r,l in zip(val_rt, val_lt) if abs(r-l) <= max_t]
    det_dist_deg=[det_dist,det_deg]
    print("det_dist_deg")
    print(det_dist_deg)
    
    return det_dist_deg

def old_dist_direc_calc(val_rt,val_lt):
#    print(val_rt)
#    print(val_lt)
    c=340.0 #[m/s]
    lr_length=0.1 #[m]
    max_t=lr_length/c*1000000 #[us]
    
    val_rt=np.array(val_rt)
    val_lt=np.array(val_lt)
    det_dist=[]
    det_deg=[]
    det_dist_deg=[]
    i=-1
    if len(val_rt)>0 and len(val_lt)>0:
        for r_t in val_rt:
            for l_t in val_lt:
                test_t_diff=r_t-l_t;
                if abs(test_t_diff) <= max_t:
                    i=i+1
                    det_dist.append(c*(r_t+l_t)/2000000/2)
                    det_deg.append(np.arcsin(c*(test_t_diff/1000000)/lr_length)/np.pi*180)
    det_dist_deg=[det_dist,det_deg]
#    det_dist_deg=np.array(det_dist_deg).T.tolist()
    print("det_dist_deg")
    print(det_dist_deg)    
    return det_dist_deg

def avoid_calc(inputs):
    k=0.6
    beta=1.6
    inputs=np.array(inputs)
    dist=inputs[0,:]
    direc=inputs[1,:]
    
    dist_fc1=np.ones(len(dist))/dist
    dist_fc2=np.sin(np.arctan2(k, dist))
    repvec_len=beta*dist_fc1*dist_fc2
    Sum_vec_late=sum(repvec_len*np.sin(np.deg2rad(direc)))
    Sum_vec_frnt=sum(repvec_len*np.cos(np.deg2rad(direc)))
    avoid = np.rad2deg(np.arctan2(-Sum_vec_late , 1 - Sum_vec_frnt ))
#    print()
#    print("avoid")
#    print(avoid)
    return avoid

def init():
    dataDir = pathlib.Path('result_sensing/')
        
    with open("condition.prm") as f:
        params = [dataDir / s.strip() for s in f.readlines()]
    return params
    
def flg_read_checker(pre_flg, trgt_flg):
    obj_file='flg.prm'
    while(pre_flg!=trgt_flg):
        try:
            with open(obj_file, mode='r') as f:
                fcntl.flock(f.fileno(), fcntl.LOCK_EX)
                pre_flg=f.readline()
                fcntl.flock(f.fileno(), fcntl.LOCK_UN)
            pre_flg=int(pre_flg)
        except:
            print("read_open error!! trgt: "+ str(trgt_flg) + "  pre:  "+ str(pre_flg))
    print("py_read_flg: " + str(pre_flg))
    print()
    return pre_flg

def flg_writer(pre_flg, trgt_flg):
    obj_file='flg.prm'
    while(pre_flg!=trgt_flg):
        try:
            with open(obj_file, mode='w') as f:
                f.write(str(trgt_flg))
            pre_flg=trgt_flg
        except:
            print("write_open error!! trgt: "+ str(trgt_flg) + "  pre:  " + str(pre_flg))
    print("py_write_flg: " + str(pre_flg))
    print()
    return pre_flg

def main():    
    flg=0
    with open('pulse_n.prm', mode='r') as f:
        pulse_n=int(float(f.readline()))
        
    print("pulse_n: " + str(pulse_n))
    time.sleep(2)

    flg=flg_read_checker(flg, 1)
#    time.sleep(1)
            
    params=init()
    p = Pool() #pool_func wo for loop no naka ni ireru to totan ni osoku naru chuui!!   
    txspc_data=p.map(fft_tx, params)
    txspc_data=txspc_data[0]
    
    for prm in params:
        with open('sensing_fpga.prm', mode='r') as f:
            file_num=f.readlines()[6]
            file_num = int(file_num.split()[0])

    flg=flg_writer(flg, 2)
#    time.sleep(1)
    
#################################loop_start
    for loop_n in range(pulse_n):
        tim_i=time.time()    
        print("")
        print("")
        print("py_pulse no: " + str(loop_n))
        
        ######################################update_check
        flg=flg_read_checker(flg, 3)
#        time.sleep(1)
    
        for prm in params:
            files = [prm / "rxwv/chk_rxwv_right_{}.dat".format(i) for i in range(file_num)]
            rxwv_r=p.map(rx_read_r,files)                    
            rxspc_data_r = p.map(fft_rx,rxwv_r)
            rxspc_data_r=rxspc_data_r[0]
            result_r =func(txspc_data,rxspc_data_r)
#            pk_det_rt = pk_det_func(result_r)    #old_ver
            pk_det_rt=pkdet_func_neo(result_r)   #new_ver
            
            files = [prm / "rxwv/chk_rxwv_left_{}.dat".format(i) for i in range(file_num)]        
            rxwv_l=p.map(rx_read_l,files)
            rxspc_data_l = p.map(fft_rx,rxwv_l)
            rxspc_data_l = rxspc_data_l[0]        
            result_l = func(txspc_data,rxspc_data_l)
#            pk_det_lt = pk_det_func(result_l)    #old_ver
            pk_det_lt=pkdet_func_neo(result_l)   #new_ver
                    
        dist_direc=dist_direc_calc(pk_det_rt,pk_det_lt)  #new_ver
#        dist_direc=old_dist_direc_calc(pk_det_rt,pk_det_lt)   #old_ver
        
#        avoid_direc=avoid_calc(dist_direc)
        dist_direc_send=''.join(map(str, dist_direc))
        dist_direc_send=dist_direc_send.replace("][", " ")
        dist_direc_send=dist_direc_send.replace("[", "")
        dist_direc_send=dist_direc_send.replace("]", "")
        dist_direc_send=dist_direc_send.replace(",", "")
                
#        ppsh.main(dist_direc_send)
        tim_f=time.time()
        print()
        print("total_t_crwv+ppsh: " + str(tim_f-tim_i))
        flg=flg_writer(flg, 4)
#        time.sleep(1)


if __name__ == '__main__':
#    tim_i=time.time()    
    main()
#    tim_f=time.time()
#    print("total_t_crwv+ppsh: " + str(tim_f-tim_i))
#    print()
    