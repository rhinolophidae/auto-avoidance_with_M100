import numpy as np
from scipy.fftpack import fft, ifft
from scipy.signal import hilbert
from multiprocessing import Pool
import pathlib
import glob
import matplotlib.pyplot as plt
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
        
    fig, (ax1, ax2) = plt.subplots(2,1, sharex=True, constrained_layout=True)
        
    with open(val) as f:
        rxwv = [float(s.strip()) for s in f.readlines()]
    crwv = ifft(fft(rxwv) * np.conj(txspc)).real
    hlbwv = hilbert(crwv)
    env = np.abs(hlbwv)  

    t = range(0, 32768)
    
    ax1.plot(t, env[t])
    ax2.plot(t, rxwv)
    ax1.set_xlim(0,15000)
    ax1.set_ylim(0,500000)
    ax2.set_ylim(-400, 400)
    i = int(str(val).split('_')[-1][:-4])
    ax1.set_title("$f_0$ = {} [kHz]  $f_1$ = {} [kHz]  T = {} [msec]".format(*(str(val.parent.parent).split('/')[-1].split('_'))))
    ax2.set_xlabel("Time [usec]")
    ax1.set_ylabel("Correlation")
    ax2.set_ylabel("Recieved Waveform")
    fig.savefig(val.parent.parent/"graph/crwv_{}.png".format(i))
    plt.show(fig)
    plt.close(fig)

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
        
    fig, (ax1, ax2) = plt.subplots(2,1, sharex=True, constrained_layout=True)
        
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
    
    ax1.plot(t, env[t])
    ax2.plot(t, rxwv)
    ax1.set_xlim(0,15000)
    ax1.set_ylim(0,500000)
    ax2.set_ylim(-400, 400)
    i = int(str(val).split('_')[-1][:-4])
    ax1.set_title("$f_0$ = {} [kHz]  $f_1$ = {} [kHz]  T = {} [msec]".format(*(str(val.parent.parent).split('/')[-1].split('_'))))
    ax2.set_xlabel("Time [usec]")
    ax1.set_ylabel("Correlation")
    ax2.set_ylabel("Recieved Waveform")
    fig.savefig(val.parent.parent/"graph/crwv_{}.png".format(i))
    plt.show(fig)
    plt.close(fig)

#    print(val)
    i = str(val).split('_')[-1][:-4]
    side = str(val).split('_')[-2]

    fname = val.parent.parent/'hilbert/chk_hilbert_{}_{}.dat'.format(side, i)
    with open(fname, mode='w') as f:
        f.write('\n'.join(map(str, hlbwv)))

    return env

def pk_det_func(inputs):
    env_thres=50000
    t_det_min=500
    t_det_max=32768
    pk_union_t=200
    
    env_over_thres_t =[i for i, x in enumerate(inputs) if x >= env_thres and t_det_min < i and i< t_det_max] #ikichi goe no id to amp wo get    
    env_pk_t=[t for t in env_over_thres_t if (inputs[t+1]-inputs[t]) * (inputs[t]-inputs[t-1])<0.0]
    env_pk_x= [x for x in inputs[env_pk_t]]
    
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
        
#    print("env_pk_t")
#    print(env_pk_t)    
#    print()
#    print("env_pk_x")
#    print(env_pk_x)
#    print()
    
    return env_pk_t


def dist_direc_calc(val_rt,val_lt):
    c=340.0 #[m/s]
    lr_length=0.1 #[m]
    max_t=lr_length/c*1000000 #[us]

    val_rt=val_rt[0]
    val_lt=val_lt[0]
    print("val_rt")
    print(val_rt)
    print("val_lt")
    print(val_lt)
    
    det_dist=[c*(r+l)/4000000 for r,l in zip(val_rt, val_lt) if abs(r-l) <= max_t]
    det_deg=[np.arcsin(c*((r-l)/1000000)/lr_length)/np.pi*180 for r,l in zip(val_rt, val_lt) if abs(r-l) <= max_t]
    det_dist_deg=[det_dist,det_deg]
    print("det_dist_deg")
    print(det_dist_deg)
    
    return det_dist_deg
    
def main():
    dataDir = pathlib.Path('result_sensing/')
    
    with open("condition.prm") as f:
        params = [dataDir / s.strip() for s in f.readlines()]

    p = Pool()
    p.map(fft_tx, params)
#    print(params)
#    print()

    for prm in params:
        (prm/'envelope').mkdir(exist_ok=True)
        (prm/'hilbert').mkdir(exist_ok=True)
        (prm/'graph').mkdir(exist_ok=True)
        
        file_num = len(list(prm.glob('rxwv/chk_rxwv_right*')))
        files = [prm / "rxwv/chk_rxwv_right_{}.dat".format(i) for i in range(file_num)]
        result_r = p.map(func_r,files)
        print("result_r")
        print(result_r)
        pk_det_rt = p.map(pk_det_func,result_r)
        
        file_num = len(list(prm.glob('rxwv/chk_rxwv_left*')))
        files = [prm / "rxwv/chk_rxwv_left_{}.dat".format(i) for i in range(file_num)]
        result_l = p.map(func_l,files)
        pk_det_lt = p.map(pk_det_func,result_l)

#        dist_direc=p.map(dist_direc_calc,pk_det_rt[0],pk_det_lt[0])
        dist_direc=dist_direc_calc(pk_det_rt,pk_det_lt)

        
if __name__ == '__main__':
    main()
