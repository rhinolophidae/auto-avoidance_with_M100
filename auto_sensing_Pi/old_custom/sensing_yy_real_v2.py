#!/usr/bin/env python
# -*- coding: utf-8 -*-

import numpy as np
from scipy import signal
from multiprocessing import Pool
import pathlib
import subprocess
import glob
import time

import os
import sys
import fcntl
#import crwv_plot_1time_real_v1 as cr

def make_pulse_data(f0, f1, T, method="lin", phi=-90):
    """
    f0, f1 -> kHz
    T -> msec で入力
    method -> リニア:"lin", 指数:"log"
    駆動パルスは元信号がthdを超えたタイミングでlow->highパルス幅pw[us]
    補助パルスは駆動パルスと同タイミングでlow->high,パルス幅pw_aux[us]
    """
    pw = 2
    pw_aux = 4
    thd = 0.5
    t = np.arange(0, int(T*10**3)) * 10**-6
    y = signal.chirp(t, f0=f0*10**3, f1=f1*10**3, t1=T*10**-3, method=method, phi=phi)
    y = y>thd
    yy = np.roll(np.logical_not(y), 1)
    z = np.logical_and(y, yy).astype(int)
    idx = np.arange(len(z))[z==1]
    drive = z.copy()
    aux = z.copy()
    for i in range(1,pw):
        drive[idx+i] = 1
    for i in range(1, pw_aux):
        aux[idx+i] = 1

    fname = "pulse_drive_data/pulse_drive_{}_{}_{}.dat".format(f0, f1, T)
    with open(fname, 'w') as f:
        f.write('\n'.join(map(str, drive)))

    fname = "pulse_drive_data/pulse_drive_aux_{}_{}_{}.dat".format(f0, f1, T)
    with open(fname, 'w') as f:
        f.write('\n'.join(map(str, aux)))

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
    return pre_flg

def main():
    flg=-1
    flg=flg_writer(flg, 0)
    
    with open('pulse_n.prm', mode='r') as f:
        pulse_n=int(f.readline())
        
    print("pulse_n: " + str(pulse_n))
    time.sleep(2)

    subprocess.call(['sudo', 'rmmod', 'ftdi_sio'])
    subprocess.call(['sudo', 'rmmod', 'usbserial'])
#    subprocess.call(['./fpga_prm.bin'])
    subprocess.call(['rm', '-f'] + glob.glob("pulse_drive_data/*dat"))
    subprocess.call(['rm', '-f', 'condition.prm'])

    params = [[80, 60, 50]]
    
    condition = ['_'.join(map(str, prm)) for prm in params]
    
    for param in params:
        with open('condition.prm', mode='w') as f:
            f.write('_'.join(map(str,param)))
            
        savedir = pathlib.Path('result_sensing/{}_{}_{}/'.format(*param))
        savedir.mkdir(exist_ok=True)
        (savedir/'rxwv').mkdir(exist_ok=True)
        [p.unlink() for p in savedir.iterdir() if p.is_file()]
        [p.unlink() for p in (savedir/'rxwv').iterdir() if p.is_file()]

        f0 = param[0] * 10**3
        f1 = param[1] * 10**3
        T = param[2] * 10**-3
        make_pulse_data(*param, method='lin')
        
    with open('condition.prm', mode='w') as f:
        f.write('\n'.join(condition))
            
    for param in params:
        subprocess.call(["sudo", "./sensing_py.bin"] + [str(p) for p in param])
        
    print('finish')
    print()
        
 
if __name__ == "__main__":
#    tim_i=time.time()
    main()
#    tim_f=time.time()
#    print("total_t: " + str(tim_f-tim_i))
#    print()
