#!/usr/bin/env python3

import subprocess
import numpy as np
import matplotlib.pyplot as plt

runs = 10

def outlier_filter(datas, threshold = 2):
    datas = np.array(datas)
    print('datas', datas)
    print('datas.std', datas.std())
    z = np.abs((datas - datas.mean()) / datas.std())
    return datas[z < threshold]

def data_processing(data_set, n):
    catgories = data_set[0].shape[0]
    samples = data_set[0].shape[1]
    final = np.zeros((catgories, samples))

    for c in range(catgories):        
        for s in range(samples):
            final[c][s] =                                                    \
                outlier_filter([data_set[i][c][s] for i in range(n)]).mean()
    return final


if __name__ == "__main__":
    Ys = []
    for i in range(runs):
        comp_proc = subprocess.run('./measure ' + str(i), shell = True)
        output = np.loadtxt('data.txt', dtype = 'float').T
        Ys.append(np.delete(output, 0, 0))
    X = output[0]
    Y = data_processing(Ys, runs)

    fig, ax = plt.subplots(1, 1, sharey = True)
    ax.set_title('perf', fontsize = 16)
    ax.set_xlabel(r'size of linked list', fontsize = 16)
    # ax.set_ylabel('time (nsec)', fontsize = 16)
    ax.set_ylabel('comparisons', fontsize = 16)

    ax.plot(X, Y[0], marker = '+', markersize = 3, label = 'list_sort')
    ax.plot(X, Y[1], marker = '*', markersize = 3, label = 'list_sort_old')
    ax.plot(X, Y[2], marker = '^', markersize = 3, label = 'timsort')
    ax.plot(X, Y[3], marker = 'x', markersize = 3, label = 'shiversort')
    ax.legend(loc = 'upper left')

    # plt.show()
    plt.savefig("result.png")