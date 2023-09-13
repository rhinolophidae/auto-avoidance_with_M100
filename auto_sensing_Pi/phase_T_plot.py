import numpy as np
import pathlib
import matplotlib.pyplot as plt


def main():
    with open("condition.prm") as f:
        params = [s.strip() for s in f.readlines()]
    # params = ["{}_{}_{}".format(f0, f1, T) for f0 in range(80, 39, -20) for f1 in range(20, 61, 20) for T in [1, 2, 5, 10, 13] if (f0 > f1) and (f0, f1)!=(80, 60)] 

    T = [int(prm.split('_')[-1]) for prm in params]
    T = sorted(set(T), key=T.index)
    fig, ax = plt.subplots(1,1, figsize=(7, 4.8), constrained_layout=True)
    means = {'-'.join(prm.split('_')[:-1])+'[kHz]':[] for prm in params}
    stds = {'-'.join(prm.split('_')[:-1])+'[kHz]':[] for prm in params}
    for prm in params:
    # for prm in ['80_20_10']:
        key = '-'.join(prm.split('_')[:-1]) + '[kHz]'
        dataDir = pathlib.Path('result_sensing') / prm

        fname = dataDir / "chk_peaks.dat"
        phases = (np.loadtxt(fname).T)[2]

        phase_mean = np.mean(phases)
        phase_std = np.std(phases)
        means[key].append(phase_mean)
        stds[key].append(phase_std)
    plots = {key: ax.errorbar(x = T, y=means[key], yerr=val, label=key, linestyle='dashed', fmt='o', capsize=3) for key, val in stds.items()}

    ax.set_xlabel("Pulse duration [msec]")
    ax.set_ylabel("Phase[rad]")
    ax.grid()
    ax.legend(bbox_to_anchor=(1.05, 1), loc='upper left', borderaxespad=0)
    plt.savefig(dataDir.parent/"phase_T.png")
    plt.show()


if __name__ == '__main__':
    main()
