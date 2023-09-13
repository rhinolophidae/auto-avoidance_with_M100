import numpy as np
from scipy.fftpack import fft, ifft
from scipy.signal import hilbert
from multiprocessing import Pool
import pathlib
import glob
import matplotlib.pyplot as plt

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
    ax1.set_xlim(0,10000)
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

    print(val)
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
    ax1.set_xlim(0,10000)
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

    print(val)
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
    print(val_rt)
    print(val_lt)
    c=340.0 #[m/s]
    lr_length=0.1 #[m]
    max_t=lr_length/c*1000000 #[us]
    
    val_rt=np.array(val_rt)
    val_lt=np.array(val_lt)
    det_dist=[]
    det_deg=[]
    det_dist_deg=[]
    i=-1
    
    for r_t in val_rt[0,:]:
        for l_t in val_lt[0,:]:
            test_t_diff=r_t-l_t;
            print(test_t_diff)
            if abs(test_t_diff) <= max_t:
                i=i+1
                det_dist.append(c*(r_t+l_t)/2000000)
                det_deg.append(np.arcsin(c*(test_t_diff/1000000)/lr_length)/np.pi*180)
                print(c*(test_t_diff)/lr_length)
    det_dist_deg=[det_dist,det_deg]
    print(det_dist_deg)
    return det_dist_deg
    
def main():
    dataDir = pathlib.Path('result_sensing/')
    
    with open("condition.prm") as f:
        params = [dataDir / s.strip() for s in f.readlines()]

    p = Pool()
    p.map(fft_tx, params)
    print(params)
    print()

    for prm in params:
        (prm/'envelope').mkdir(exist_ok=True)
        (prm/'hilbert').mkdir(exist_ok=True)
        (prm/'graph').mkdir(exist_ok=True)
        
        file_num = len(list(prm.glob('rxwv/chk_rxwv_right*')))
        files = [prm / "rxwv/chk_rxwv_right_{}.dat".format(i) for i in range(file_num)]
        result_r = p.map(func_r,files)
        pk_det_rt=p.map(pk_det_func,result_r)
        
        file_num = len(list(prm.glob('rxwv/chk_rxwv_left*')))
        files = [prm / "rxwv/chk_rxwv_left_{}.dat".format(i) for i in range(file_num)]
        result_l = p.map(func_l, files)
        pk_det_lt=p.map(pk_det_func,result_l)

        dist_direc=dist_direc_calc(pk_det_rt,pk_det_lt)

        
if __name__ == '__main__':
    main()
