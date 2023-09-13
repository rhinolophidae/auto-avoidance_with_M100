#!/usr/bin/env python
# -*- coding: utf-8 -*-


#"""/home/pi/Program/sensingtest_20210211_mimic_chirp/gtk""" ni aru fpga_prm.c de settei henkou kanou

import numpy as np
import math
from scipy import signal
from multiprocessing import Pool
import pathlib
import subprocess
import glob
import time

import os
import sys
import fcntl
#import crwv_plot_1time_mimic_v2 as crmmc

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

def mimic_func2(Fs, Amp, f_init, f_termin, Duration, t, bcc, t_W):
    ###mimic_variables###
    fr = Fs  #1000000  #Sampling rate [Hz]
    amp = Amp #10.0    #Amplitude: 32767/2
    fStart = f_init #100000    #: Start frequency [Hz]
    fEnd = f_termin #40000  #: End frequency [Hz]
    dur = Duration #0.02    #: Call duration [s]

    ###control_param###
    BatCallConstant = bcc   #0.1 #0.005 (1-0.000001 x0.1digit)
    t_wait_param = t_W   #20 #(100-10 5_digit)
    arg = BatCallConstant * fEnd/fStart
    wv= amp * np.sin(2.*np.pi*(fStart/(fStart-BatCallConstant*fEnd))*((fStart-fEnd)/t_wait_param*np.power(arg, t*t_wait_param)/np.log(arg)+(1-BatCallConstant)*fEnd*t))
    return wv

def LPM_func(Fs, Amp, f_init, f_termin, Duration, t):
    Ts=1.0/Fs #[Hz]sampling_period
    xi= 1#arbitary num?(not 0)
    alpha=2*np.pi*xi*f_init    
    gamma=(f_init-f_termin)/(f_termin*Duration)*xi
    f=alpha/(2*np.pi*(gamma*t+xi))
    del_phase = 2*np.pi*f*Ts
    return del_phase

def CF_add(Fs, Amp, f_termin, t):
    Ts=1.0/Fs #[Hz]sampling_period
    f=f_termin*np.ones(int(len(t)))
    del_phase = 2*np.pi*f*Ts
    return del_phase

def trans_wv(del_phase_array):
    phase=0
    wave=np.zeros(len(del_phase_array))
    
    for i in range(len(del_phase_array)):
        phase = phase +del_phase_array[i]
        wave[i] = np.sin(phase)
    return wave

def make_insp_pulse_data(f0, f1, T, log_BCC, tw, T_CF, method, Data_size, phi=-90):
    p=Pool()
    BCC=np.power(10.0,log_BCC)
    print()
    print("method:", method, "BCC: ", BCC, "   twait: ", tw)
    print()
    
    """
    f0, f1 -> kHz
    T -> msec
    method -> Kubota_Denis_func_eq:"mimic"
    """    
    pw = 2
    pw_aux = 8 #ofset, =4
    pw_reftx = 8 #90-50kHz nara 10us ga saiteki, on-pulse_duration_for_Xcor_tx
    thd = 0.5
    
    t = np.arange(0, int(T*10**3)) * 10**-6
    t_CF = np.arange(0, int(T_CF*10**3)) * 10**-6
    
    if method == "mimic":
        y = mimic_func2(1000000, 1.0, f0*10**3, f1*10**3, T*10**-3, t, BCC, tw) #(fs, Amp, f0, f1, T, t, BCC, tw)
        y=np.array(y)
    elif method == "lin":
        y = signal.chirp(t, f0=f0*10**3, f1=f1*10**3, t1=T*10**-3, method=method, phi=phi)
    elif method == "LPM":
        y_del_phase = LPM_func(1000000, 1.0, f0*10**3, f1*10**3, T*10**-3, t) #(fs, Amp, f0, f1, T, t)
        y_del_phase_CF = CF_add(1000000, 1.0, f1*10**3, t_CF) #(fs, Amp, f0, f1, T, t)
        y_del_phase =np.hstack([y_del_phase, y_del_phase_CF])
        y = trans_wv(y_del_phase)
        y = np.array(y)
    
    y = y>thd # DC seibun wo irete baion wo kaseide iru?    
    yy = np.roll(np.logical_not(y), 1)
    z = np.logical_and(y, yy).astype(int)
    idx = np.arange(len(z))[z==1] #1 ga hairu hakono banchi wo yomitotte iru
    drive = z.copy()
    aux = z.copy()
    reftx = z.copy()
    
    for i in range(1,pw):
        try:#saidai_hakosuu wo koeruto error ga derunode try wo ireta
            drive[idx+i] = 1
        except:
            pass
    for i in range(1, pw_aux):
        try:
            aux[idx+i] = 1
        except:
            pass
        
    for i in range(1, pw_reftx):
        try:
            reftx[idx+i] = 1
        except:
            pass
        
    reftx = reftx * 2-1
    reftx = np.hstack((reftx, np.zeros(Data_size-len(reftx))))
    print("reftx_size", np.size(reftx))
    
    fname = "pulse_drive_data/pulse_drive_{}_{}_{}.dat".format(f0, f1, T)
    with open(fname, 'w') as f:
        f.write('\n'.join(map(str, drive)))
    fname = "pulse_drive_data/pulse_drive_aux_{}_{}_{}.dat".format(f0, f1, T)
    with open(fname, 'w') as f:
        f.write('\n'.join(map(str, aux)))
    fname = "result_sensing/{}_{}_{}_{}_{}/chk_txwv_ref.dat".format(f0, f1, T, log_BCC, tw)
    with open(fname, 'w') as f:
        f.write('\n'.join(map(str, reftx)))


def main():
    p=Pool() 
    params = [[90, 50, 45]]    #init_f term_f duration
    T_CF=0
    data_size = 2**16
    condition = ['_'.join(map(str, prm)) for prm in params]
        
    method ="LPM"
    
    if method == "mimic":
        ###### mimicking_parameter_search_setting #########
        BCC_Min = 0.01 #0.0001(10) 0.00001(22) 0.01(50)
        BCC_Max = BCC_Min #0.0001(10) 0.00001(22) 0.01(50)
        T_wait_min = 20 #70(10) 30(22) 20(50)
        T_wait_max = T_wait_min #70(10) 30(22) 20(50)
        T_wait_digit = 10
    
    #    BCC_Array = np.geomspace(BCC_Min, BCC_Max, int(np.log10(BCC_Max / BCC_Min))+1)
        BCC_log_Array = np.arange(int(np.log10(BCC_Min)), int(np.log10(BCC_Max))+1, 1)
        T_wait_Array = np.arange(T_wait_min, T_wait_max+1, T_wait_digit)
        
    elif method == "lin":
        BCC_log_Array = p.map(int,np.zeros(1)) #dummy_value_inserted
        T_wait_Array = p.map(int,np.zeros(1)) #dummy_value_inserted
        
        print(T_wait_Array)
        print("method == linear!")
        
    elif method == "LPM":
        BCC_log_Array = p.map(int,np.zeros(1)) #dummy_value_inserted
        T_wait_Array = p.map(int,np.zeros(1)) #dummy_value_inserted
        
        print(T_wait_Array)
        print("method == LPM!")
        
    print("BCC_log_Array: ", BCC_log_Array, " T_wait_Array: ", T_wait_Array)
    print()
        
    #####################################################
    
    for twait in T_wait_Array:
        for BatCallConst in BCC_log_Array:
            
            flg=-1
            flg=flg_writer(flg, 0)
    
            with open('pulse_n.prm', mode='r') as f:
                pulse_n=int(f.readline())
        
            print("pulse_n: " + str(pulse_n))
            time.sleep(2)

            subprocess.call(['sudo', 'rmmod', 'ftdi_sio'])
            subprocess.call(['sudo', 'rmmod', 'usbserial'])
#            subprocess.call(['./fpga_prm.bin'])
            subprocess.call(['rm', '-f'] + glob.glob("pulse_drive_data/*dat"))
            subprocess.call(['rm', '-f', 'condition.prm'])
            
            if len(params[0])==3:
                params = np.append(params, [[BatCallConst, twait]], axis=1)
            if len(params[0])==5:
                params[0][3] = BatCallConst
                params[0][4] = twait
            print()
            print("params: ", params, " condition:", condition, " len(params[0])", len(params[0]))
                        
            for prm in params:
                with open('condition.prm', mode='w') as f:
                    f.write('_'.join(map(str,prm)))
            
                savedir = pathlib.Path('result_sensing/{}_{}_{}_{}_{}/'.format(*prm))
                savedir.mkdir(exist_ok=True)
                (savedir/'rxwv').mkdir(exist_ok=True)
                [p.unlink() for p in savedir.iterdir() if p.is_file()]
                [p.unlink() for p in (savedir/'rxwv').iterdir() if p.is_file()]

                f0 = prm[0] * 10**3
                f1 = prm[1] * 10**3
                T = prm[2] * 10**-3
                
                make_insp_pulse_data(*prm, T_CF, method, data_size)
#                if method == "LPM":
#                    make_insp_pulse_data(*prm, T_CF, "LPM")
#                if method == "lin":
#                    make_insp_pulse_data(*prm, T_CF, "lin")
#                elif method == "mimic":
#                    make_insp_pulse_data(*prm, T_CF, "mimic")
            
            for prm in params:
                subprocess.call(["sudo", "./sensing_py.bin"] + [str(p) for p in prm])
        
            print('finish')
            print()
#            crmmc.main()
            
 
if __name__ == "__main__":
    main()
