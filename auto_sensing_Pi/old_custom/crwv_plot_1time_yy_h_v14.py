import numpy as np
from scipy.fftpack import fft, ifft
from scipy.signal import hilbert
from multiprocessing import Pool
import pathlib
import glob
import matplotlib.pyplot as plt
import pypysocket_host_v02 as ppsh
import time

def fft_tx(val):
    fname = val / 'chk_txwv.dat'
    with open(fname) as f:
        txwv = [float(s.strip()) for s in f.readlines()]

    txspc = fft(txwv)
    
    fname = val / 'chk_txspc.dat'
    with open(fname, mode='w') as f:
        f.write('\n'.join(map(str, txspc)))
        

def func_l(val):
    fname = val.parent.parent / "chk_txspc.dat"
    with open(fname) as f:
        txspc = [complex(s.strip()) for s in f.readlines()]
        
#    fig, (ax1, ax2) = plt.subplots(2,1, sharex=True, constrained_layout=True)
        
    with open(val) as f:
        rxwv = [float(s.strip()) for s in f.readlines()]
    crwv = ifft(fft(rxwv) * np.conj(txspc)).real
    hlbwv = hilbert(crwv)
    env = np.abs(hlbwv)  

    t = range(0, 32768)
#    print(val)
    i = str(val).split('_')[-1][:-4]
    side = str(val).split('_')[-2]

    fname = val.parent.parent/'hilbert/chk_hilbert_{}_{}.dat'.format(side, i)
    with open(fname, mode='w') as f:
        f.write('\n'.join(map(str, hlbwv)))

    return env

def func_r(val):
    fname = val.parent.parent / "chk_txspc.dat"
    with open(fname) as f:
        txspc = [complex(s.strip()) for s in f.readlines()]
#    fig, (ax1, ax2) = plt.subplots(2,1, sharex=True, constrained_layout=True)
        
    with open(val) as f:
        rxwv = [float(s.strip()) for s in f.readlines()]
    ################### DC_cut ############################
    list_correc = np.ones(len(rxwv))*14.0
    rxwv =np.array(rxwv)
    rxwv=rxwv-list_correc 
    rxwv=rxwv.tolist()
    ################### DC_cut ############################
    crwv = ifft(fft(rxwv) * np.conj(txspc)).real
    hlbwv = hilbert(crwv)
    env = np.abs(hlbwv)  

    t = range(0, 32768)
#    print(val)
    i = str(val).split('_')[-1][:-4]
    side = str(val).split('_')[-2]

    fname = val.parent.parent/'hilbert/chk_hilbert_{}_{}.dat'.format(side, i)
    with open(fname, mode='w') as f:
        f.write('\n'.join(map(str, hlbwv)))

    return env

def pk_det_func(inputs):
    env_thress=200000
    env_check=inputs
    env_upper_thress =[i for i, x in enumerate(env_check) if x >= env_thress]
    
    env_diff_det =np.sign(np.diff(env_check))
    env_diff_pre=np.insert(env_diff_det, 0, 0)
    env_diff_post=np.append(env_diff_det, 0)
    env_prepost_diff = [x * y for (x, y) in zip(env_diff_pre, env_diff_post)]
    env_peak =[i for i, x in enumerate(env_prepost_diff) if x < 0]
    echo_t = sorted(set(env_upper_thress) & set(env_peak))
    echo_t = [i for i in echo_t if i >= 280]
#    print(echo_t)
    return echo_t

def dist_direc_calc(val_rt,val_lt):
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
        for r_t in val_rt[0,:]:
            for l_t in val_lt[0,:]:
                test_t_diff=r_t-l_t;
                if abs(test_t_diff) <= max_t:
                    i=i+1
                    det_dist.append(c*(r_t+l_t)/2000000)
                    det_deg.append(np.arcsin(c*(test_t_diff/1000000)/lr_length)/np.pi*180)
    det_dist_deg=[det_dist,det_deg]
#    det_dist_deg=np.array(det_dist_deg).T.tolist()
#    print("det_dist_deg")
#    print(det_dist_deg)
    
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
    print(avoid)

    return avoid

def main():
    tim0=time.time()

    dataDir = pathlib.Path('result_sensing/')
    
    tim1=time.time()
    print("tim10")
    print(tim1-tim0)
    
    with open("condition.prm") as f:
        params = [dataDir / s.strip() for s in f.readlines()]

    p = Pool()
    p.map(fft_tx, params)
#    print(params)

    print("tim21")
    tim2=time.time()
    print(tim2-tim1)

    for prm in params:
#        (prm/'envelope').mkdir(exist_ok=True)
#        (prm/'hilbert').mkdir(exist_ok=True)
#        (prm/'graph').mkdir(exist_ok=True)
        
        file_num = len(list(prm.glob('rxwv/chk_rxwv_right*')))
        files = [prm / "rxwv/chk_rxwv_right_{}.dat".format(i) for i in range(file_num)]
        result_r = p.map(func_r,files)
        pk_det_rt=p.map(pk_det_func,result_r)
        
        file_num = len(list(prm.glob('rxwv/chk_rxwv_left*')))
        files = [prm / "rxwv/chk_rxwv_left_{}.dat".format(i) for i in range(file_num)]        
        result_l = p.map(func_l, files)
        pk_det_lt=p.map(pk_det_func,result_l)

        print("result")
        print(result_l)
        print("result[0]")
        print(result_l[0])
        print("result[00]")
        print(result_l[0][0])
        print("type(result)")
        print(type(result_l))
        print("type(result[0])")
        print(type(result_l[0]))
        print("type(result[00])")
        print(type(result_l[0][0]))
        time.sleep(30)
    
    print("tim32")
    tim3=time.time()
    print(tim3-tim2)
"""    
    dist_direc=dist_direc_calc(pk_det_rt,pk_det_lt)
    print("tim43")
    tim4=time.time()
    print(tim4-tim3)

    avoid_direc=avoid_calc(dist_direc)
    dist_direc_send=''.join(map(str, dist_direc))
    dist_direc_send=dist_direc_send.replace("][", " ")
    dist_direc_send=dist_direc_send.replace("[", "")
    dist_direc_send=dist_direc_send.replace("]", "")
    dist_direc_send=dist_direc_send.replace(",", "")
    
    tim5=time.time()
    print("tim54")
    print(tim5-tim4)
    
    ppsh.main(dist_direc_send)

    tim6=time.time()
    print("tim65")
    print(tim6-tim5)
    
    print("tim60")
    print(tim6-tim0)
"""    
if __name__ == '__main__':
    main()
