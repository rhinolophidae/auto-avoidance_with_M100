import numpy as np
from scipy.fftpack import fft, ifft
from scipy.signal import hilbert, argrelmax
from multiprocessing import Pool
import pathlib
import glob
import pypysocket_host_1timcon_v04 as ppsh
import time

def fft_tx(val):
    fname = val / 'chk_txwv.dat'
    with open(fname) as f:
        txwv = [s for s in map(float,f.readlines())]
#        txwv = [float(s) for s in f.readlines()]
#        txwv = [float(s.strip()) for s in f.readlines()]
    txspc = fft(txwv)
        
    return txspc


def rx_read_r(val):
    with open(val) as f:
        rxwv = [s for s in map(float,f.readlines())]
#        rxwv = [float(s) for s in f.readlines()]
#        rxwv = [float(s.strip()) for s in f.readlines()]
    ################### DC_cut ############################
    DC_correc = np.full(len(rxwv),14.0)
    rxwv =np.array(rxwv)
    rxwv=rxwv-DC_correc 
    ################### DC_cut ############################
    return rxwv

def rx_read_l(val):
    with open(val) as f:
        rxwv = [s for s in map(float,f.readlines())]
#        rxwv = [float(s) for s in f.readlines()]
#        rxwv = [float(s.strip()) for s in f.readlines()]
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

def pkdet_func_neo(inputs):    
    env_thress=300000
    t_wait=280
    env_check=inputs
    
    #new_calc
    peaks_t=np.array(argrelmax(inputs))[0]
    echo_t=peaks_t[env_check[peaks_t]>env_thress]
    echo_t=echo_t[echo_t>t_wait]
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
#    print(avoid)

    return avoid

def test(val):
    val2=np.conj(val)
    return val

def main():
    #tim0=#time.#time()
    for i in range(100):
        dataDir = pathlib.Path('result_sensing/')
        
        
        with open("condition.prm") as f:
            params = [dataDir / s.strip() for s in f.readlines()]

        p = Pool()
        txspc_data=p.map(fft_tx, params)
        txspc_data=txspc_data[0]

        ##tim2=#time.#time()
        #print("tx_FFT")
        #print(#tim2-#tim0)

        for prm in params:

            file_num = len(list(prm.glob('rxwv/chk_rxwv_right*')))
            files = [prm / "rxwv/chk_rxwv_right_{}.dat".format(i) for i in range(file_num)]
            rxwv_r=p.map(rx_read_r,files)
            
            ##tim3=#time.#time()
            #print("read_r")
            #print(#tim3-#tim2)
            
            rxspc_data_r = p.map(fft_rx,rxwv_r)
            rxspc_data_r=rxspc_data_r[0]
            
            ##tim4=#time.#time()
            #print("rx_FFT")
            #print(#tim4-#tim3)
            
            result_r =func(txspc_data,rxspc_data_r)

            ##tim5=#time.#time()
            #print("Xcor")
            #print(#tim5-#tim4)
                
            pk_det_rt=pkdet_func_neo(result_r)
                    
            ##tim6=#time.#time()
            #print("peak_detection")
            #print(#tim6-#tim5)
            
            file_num = len(list(prm.glob('rxwv/chk_rxwv_left*')))
            files = [prm / "rxwv/chk_rxwv_left_{}.dat".format(i) for i in range(file_num)]        
            rxwv_l=p.map(rx_read_l,files)
            rxspc_data_l = p.map(fft_rx,rxwv_l)
            rxspc_data_l = rxspc_data_l[0]        
            result_l = func(txspc_data,rxspc_data_l)
            pk_det_lt=pkdet_func_neo(result_l)
                    
        ##tim7=#time.#time()
        
        dist_direc=dist_direc_calc(pk_det_rt,pk_det_lt)
        
        ##tim8=#time.#time()
        #print("dist_direc_calc")
        #print(#tim8-#tim7)
        
        avoid_direc=avoid_calc(dist_direc)
        dist_direc_send=''.join(map(str, dist_direc))
        dist_direc_send=dist_direc_send.replace("][", " ")
        dist_direc_send=dist_direc_send.replace("[", "")
        dist_direc_send=dist_direc_send.replace("]", "")
        dist_direc_send=dist_direc_send.replace(",", "")
        
        #tim9=#time.#time()
        
        ppsh.main(dist_direc_send)
        print(i)

        #tim10=#time.#time()
        #print("send_data_to_flight con")
        #print(#tim10-#tim9)
        
        #print("total")
        #print(#tim10-#tim0)
        
if __name__ == '__main__':
    main()
