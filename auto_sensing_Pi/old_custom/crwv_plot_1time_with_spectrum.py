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

def func(val):
    fname = val.parent.parent / "chk_txspc.dat"
    with open(fname) as f:
        txspc = [complex(s.strip()) for s in f.readlines()]
        
    fig, (ax1, ax2, ax3) = plt.subplots(3,1, sharex=True, constrained_layout=True)
        
    with open(val) as f:
        rxwv = [float(s.strip()) for s in f.readlines()]
    crwv = ifft(fft(rxwv) * np.conj(txspc)).real
    rxspc=fft(rxwv)
    hlbwv = hilbert(crwv)
    env = np.abs(hlbwv)  
    t = range(0, 32768)

    ax1.plot(t, env[t])
    ax2.plot(t, rxwv)
    ax3.plot(t, rxspc)
    ax1.set_xlim(0,10000)
    ax1.set_ylim(0,500000)
    ax2.set_ylim(-400, 400)
    ax3.set_ylim(0, 500000)
    i = int(str(val).split('_')[-1][:-4])
    ax1.set_title("$f_0$ = {} [kHz]  $f_1$ = {} [kHz]  T = {} [msec]".format(*(str(val.parent.parent).split('/')[-1].split('_'))))
    ax2.set_xlabel("Time [usec]")
    ax3.set_xlabel("freq [kHz]")
    ax1.set_ylabel("Correlation")
    ax2.set_ylabel("Recieved Waveform")
    ax3.set_ylabel("Spector")
    fig.savefig(val.parent.parent/"graph/crwv_{}.png".format(i))
    plt.show(fig)
    plt.close(fig)

    print(val)
    i = str(val).split('_')[-1][:-4]
    side = str(val).split('_')[-2]

    fname = val.parent.parent/'hilbert/chk_hilbert_{}_{}.dat'.format(side, i)
    with open(fname, mode='w') as f:
        f.write('\n'.join(map(str, hlbwv)))

    return hlbwv

def main():
    dataDir = pathlib.Path('result_sensing/')
    
    with open("condition.prm") as f:
        params = [dataDir / s.strip() for s in f.readlines()]

    p = Pool()
    p.map(fft_tx, params)

    for prm in params:
        print(prm)
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
