import numpy as np
from scipy.fftpack import fft, ifft
from scipy.signal import hilbert
from multiprocessing import Pool
import pathlib
import glob
import pypysocket_host_v02 as ppsh
import time

def fft_tx(val):
    fname = val / 'chk_txwv.dat'
    with open(fname) as f:
        txwv = [float(s.strip()) for s in f.readlines()]
    txspc = fft(txwv)
    
    return txspc

def rx_read_r(val):
    with open(val) as f:
        rxwv = [float(s.strip()) for s in f.readlines()]
    ################### DC_cut ############################
    DC_correc = np.ones(len(rxwv))*14.0
    rxwv =np.array(rxwv)
    rxwv=rxwv-DC_correc 
#    rxwv=rxwv.tolist()
    ################### DC_cut ############################
    return rxwv

def rx_read_l(val):
    with open(val) as f:
        rxwv = [float(s.strip()) for s in f.readlines()]
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
        for r_t in val_rt:
            for l_t in val_lt:
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

def test(val):
    val2=np.conj(val)
    return val

def main():
    tim0=time.time()

    dataDir = pathlib.Path('result_sensing/')
    
    tim1=time.time()
    print("tim10")
    print(tim1-tim0)
    
    with open("condition.prm") as f:
        params = [dataDir / s.strip() for s in f.readlines()]

    p = Pool()
    txspc_data=p.map(fft_tx, params)
    txspc_data=txspc_data[0]

    tim2=time.time()
    print("tim21")
    print(tim2-tim1)

    for prm in params:

        file_num = len(list(prm.glob('rxwv/chk_rxwv_right*')))
        files = [prm / "rxwv/chk_rxwv_right_{}.dat".format(i) for i in range(file_num)]
        rxwv_r=p.map(rx_read_r,files)
        rxspc_data_r = p.map(fft_rx,rxwv_r)
        rxspc_data_r = rxspc_data_r[0]
        result_r =func(txspc_data,rxspc_data_r)
        pk_det_rt=pk_det_func(result_r)
                
        file_num = len(list(prm.glob('rxwv/chk_rxwv_left*')))
        files = [prm / "rxwv/chk_rxwv_left_{}.dat".format(i) for i in range(file_num)]        
        rxwv_l=p.map(rx_read_l,files)
        rxspc_data_l = p.map(fft_rx,rxwv_l)
        rxspc_data_l = rxspc_data_l[0]        
        result_l = func(txspc_data,rxspc_data_l)
        pk_det_lt=pk_det_func(result_l)
                
    print("tim32")
    tim3=time.time()
    print(tim3-tim2)
    
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
    
if __name__ == '__main__':
    main()
