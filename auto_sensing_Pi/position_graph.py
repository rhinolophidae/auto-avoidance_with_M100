import numpy as np
import matplotlib.pyplot as plt
from sympy import divisors
import pathlib


c = 340.
fps = 60.

def main():
    dataDir = pathlib.Path("result_sensing")
    with open("condition.prm") as f:
        params = [s.strip() for s in f.readlines()]
    points = [3,5,7,9]
    fig, ax = plt.subplots(1, 1)
    ax.set_xlabel("time [sec]")
    ax.set_ylabel("position [mm]")
    ax.grid()
    pos_plot, = ax.plot([])
    for prm in params:
        fileDir = dataDir/prm
        with open(fileDir/"chk_peaks.dat") as f:
            ptime = float(f.readline().strip().split(" ")[0])
        x0 = c/2. * ptime * 10**-3
        for k in points:
            with open(fileDir/"displacement_{}.dat".format(k)) as f:
                dx = [float(s.strip().split(" ")[0]) for s in f.readlines()]
            pos = np.cumsum([x0] + dx)
            t = np.arange(0, len(pos))/fps
            pos_plot.set_data(t, pos)
            ax.set_xlim(t[0], t[-1])
            ax.set_ylim(pos[0]-.3, pos[0]+.3)
            ax.set_title("$f_0$ = {} [kHz], $f_1$ = {} [kHz], T = {} [msec], points = {}".format(*(prm.split("_")), k))
            plt.savefig(fileDir/"position_{}.png".format(k))
    plt.show()


if __name__ == '__main__':
    main()
