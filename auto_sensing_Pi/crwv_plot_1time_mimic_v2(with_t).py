import numpy as np
from scipy.fftpack import fft, ifft
from scipy.signal import hilbert
from multiprocessing import Pool
import pathlib
import glob
import matplotlib.pyplot as plt
import os

def fft_tx(val):
    fname = val / 'chk_txwv_ref.dat'
    
    with open(fname) as f:
        txwv = [float(s.strip()) for s in f.readlines()]
    print("len(txwv)", len(txwv))
    txspc = fft(txwv)
    fname = val / 'chk_txspc.dat'
    with open(fname, mode='w') as f:
        f.write('\n'.join(map(str, txspc)))

def thres_func(t_us):
    BOXNUM=2**16
    alpha=1600000 #1300000
    sigma=BOXNUM /35 #/ 13
    thd_base=100000
    thd = np.maximum(thd_base, alpha*np.exp(-t_us/sigma))
    return thd

def func(val):
    fname = val.parent.parent / "chk_txspc.dat"
    with open(fname) as f:
        txspc = [complex(s.strip()) for s in f.readlines()]
        
    fig, (ax1, ax2) = plt.subplots(2,1, sharex=True, constrained_layout=True)
        
    with open(val) as f:
        rxwv = [s.strip() for s in f.readlines()]
        time=rxwv[0]
        rxwv=rxwv[1::]
        rxwv = [float(s) for s in rxwv]
        rx_ave = np.average(rxwv)
        print("rx_ave=", rx_ave)
        rxwv = rxwv-rx_ave
    print("len(rxwv)", len(rxwv))
    crwv = ifft(fft(rxwv) * np.conj(txspc)).real
    hlbwv = hilbert(crwv)
    env = np.abs(hlbwv)  

    t0 = np.arange(len(env))
    thd_array = thres_func(t0)
    t =[i / 1000 for i in range(0, len(env))]
    ax1.plot(t, rxwv)
    ax2.plot(t, env[t0])
    ax2.plot(t, thd_array)
    ax2.set_xlim(0,max(t))
    ax2.set_ylim(0,1500000)
#    ax2.set_ylim(0,500000)
    ax1.set_ylim(-800, 800)
    i = int(str(val).split('_')[-1][:-4])
    side = str(val).split('_')[-2]
    
    ax1.set_title("$f_0$ = {} [kHz]  $f_1$ = {} [kHz]  T = {} [msec]".format(*(str(val.parent.parent).split('/')[-1].split('_'))))#LINUX
#    ax1.set_title("$f_0$={}[kHz]  $f_1$={}[kHz]  T={}[msec]  k1={} k2={}".format(*(str(val).split('\\')[1].split('_'))))#WIN
    ax2.set_xlabel("Time [msec]")
    ax2.set_ylabel("Correlation")
    ax1.set_ylabel("Recieved Waveform")
    

    fig.savefig(val.parent.parent/"graph/crwv_{}_{}.png".format(side,i))
    print(val)
    
    plt.pause(0.2)
#    plt.close(fig)
    plt.clf()
    plt.close()

    fname = val.parent.parent/'hilbert/chk_hilbert_{}_{}.dat'.format(side, i)
    with open(fname, mode='w') as f:
        f.write('\n'.join(map(str, hlbwv)))

    fname = val.parent.parent/'envelope/chk_env_{}_{}.dat'.format(side, i)
    with open(fname, mode='w') as f:
        f.write('\n'.join(map(str, env)))

    return hlbwv

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
        
        file_num = len(list(prm.glob('rxwv/chk_rxwv_right*')))
        files = [prm / "rxwv/chk_rxwv_right_{}.dat".format(i) for i in range(file_num)]
        result = p.map(func, files)      

        file_num = len(list(prm.glob('rxwv/chk_rxwv_left*')))
        files = [prm / "rxwv/chk_rxwv_left_{}.dat".format(i) for i in range(file_num)]
        result = p.map(func, files)
        
if __name__ == '__main__':
    main()

