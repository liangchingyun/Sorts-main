#!/usr/bin/env python3

import subprocess
import numpy as np
import scipy.stats

runs = 500

def get_p_value(f_value, df1, df2):
    return 1 - scipy.stats.f.cdf(f_value, df1, df2)

def hypothesis_testing(p_value):
    if p_value < 0.05:
        return True
    else:
        return False

sortType = {
    'list_sort': 0,
    'list_sort_old': 1,
    'timsort': 2,
    'shiversort': 3
}

def get_f_value(data, sort1, sort2):
    arr1 = []
    arr2 = []
    for i in range(runs):
        arr1.append(data[i][sort1])
        arr2.append(data[i][sort2])
    arr1 = np.array(arr1)
    arr2 = np.array(arr2)
    n1 = runs
    n2 = runs
    mu1 = arr1.mean()
    mu2 = arr2.mean()
    print('mu1', mu1)
    print('mu2', mu2)
    mu = (mu1 + mu2) / 2
    std1 = arr1.std()
    std2 = arr2.std()
    df1 = 1
    df2 = runs * 2 - 2
    MSTR = n1 * (mu1 - mu) ** 2 + n2 * (mu2 - mu) ** 2
    MSE = ((n1 - 1) * std1 ** 2 + (n2 - 1) * std2 ** 2) / df2

    return MSTR / MSE

if __name__ == "__main__":
    Ys = [] ## [runs, size, sort]
    for i in range(runs):
        comp_proc = subprocess.run('./measure ' + str(i+1), shell = True)
        output = np.loadtxt('data.txt', dtype = 'float').T
        print(output)
        Ys.append(np.delete(output, 0, 0))
    size = output[0]
    print("Testing linked list of size " + str(size))
    print('')
    for sort1 in sortType:
        for sort2 in sortType:
            if sort1 == sort2:
                break
            print('Comparing ' + sort1 + ' and ' + sort2)
            df1 = 1
            df2 = runs * 2 - 2
            f_value = get_f_value(Ys, sortType[sort1], sortType[sort2])
            p_value = get_p_value(f_value, df1, df2)
            print('f_value', f_value)
            print('p_value', p_value)
            rejectH0 = hypothesis_testing(p_value)
            if rejectH0:
                print("Statistically significant difference between " + sort1 + " and " + sort2)
                print('')
            else:
                print("No statistically significant difference between " + sort1 + " and " + sort2)
                print('')