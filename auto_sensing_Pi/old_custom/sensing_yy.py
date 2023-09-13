import numpy as np
from scipy import signal
import pathlib
import subprocess
import glob
import time
import crwv_plot_1time_yy_hosei as cr
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


def main():
    subprocess.call(['sudo', 'rmmod', 'ftdi_sio'])
    subprocess.call(['sudo', 'rmmod', 'usbserial'])
    subprocess.call(['./fpga_prm.bin'])
    subprocess.call(['rm', '-f'] + glob.glob("pulse_drive_data/*dat"))
    subprocess.call(['rm', '-f', 'condition.prm'])

    # params = [[f0, f1, T] for f0 in range(80, 39, -20) for f1 in range(20, 61, 20) for T in [1, 2, 5, 10, 13] if f0 > f1]
    # params = [[80, 20, 10],[80, 40, 10]]
    params = [[80, 40, 20]]
    
    condition = ['_'.join(map(str, prm)) for prm in params]

    for param in params:
        print(param)
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

        subprocess.call(["sudo", "./sensing_py.bin"] + [str(p) for p in param]) 
    with open('condition.prm', mode='w') as f:
        f.write('\n'.join(condition))
    cr.main()

if __name__ == "__main__":
    main()
