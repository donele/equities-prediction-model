import argparse
import sys
import pickle
import glob
import subprocess
import collections.abc
from pathlib import PurePosixPath
import numpy as np

def getConcSigDir():
    return '/home/jelee/hffit/CA/concSig/om/u0'

def get_filenames(dfrom, dto, data_type):
    concSigDir = getConcSigDir()
    pat = '{}/concurrent_{}_*.csv'.format(concSigDir, data_type)
    all_files = glob.glob(pat)
    filenames = []
    for filename in all_files:
        dloc = filename.find('201')
        idate = int(filename[dloc:(dloc+8)])
        if idate >= int(dfrom) and idate <= int(dto):
            filenames.append(filename)
    return filenames

def get_meta(filenames):
    idates = []
    for filename in filenames:
        dloc = filename.find('201')
        idate = int(filename[dloc:(dloc+8)])
        idates.append(idate)

    tickers = []
    concSigDir = getConcSigDir()
    tickerFile = glob.glob('{}/tickerList*'.format(concSigDir))
    with open(tickerFile[0], 'r') as f:
        for line in f:
            line = line.strip().split()
            ticker = line[0]
            tickers.append(ticker)

    timestamps = []
    timeFile = glob.glob('{}/*meta*'.format(concSigDir))
    with open(timeFile[0], 'r') as f:
        for line in f:
            sec = line.strip()
            timestamps.append(sec)

    return idates, tickers, timestamps

def iter_loadtxt(filenames, delimiter=',', skiprows=0, dtype=float, nvars=None):
    iter_loadtxt.rowlength = 0
    def iter_func():
        for filename in filenames:
            with open(filename, 'r') as f:
                for _ in range(skiprows):
                    next(f)
                for line in f:
                    line = line.rstrip().split(delimiter)
                    for item in line:
                        yield dtype(item)
                #iter_loadtxt.rowlength = int(len(line)/nvars)
                if nvars is None:
                    iter_loadtxt.rowlength = len(line)
                else:
                    iter_loadtxt.rowlength = int(len(line)/nvars)

    data = np.fromiter(iter_func(), dtype=dtype, count=-1)
    if nvars is None:
        data = data.reshape((-1, iter_loadtxt.rowlength))
        return data
    else:
        data = data.reshape((-1, iter_loadtxt.rowlength, nvars))
        return data
    return None

def loadtxt(filenames, delimiter=',', skiprows=0, dtype=float, nvars=None):
    data = []
    len_row = 0
    for filename in filenames:
        with open(filename, 'r') as f:
            for _ in range(skiprows):
                next(f)
            for line in f:
                line = line.rstrip().split(delimiter)
                row = [float(x) for x in line]
                data.extend(row)

                if nvars is None:
                    len_row = len(row)
                else:
                    len_row = int(len(row)/nvars)
    if nvars is None:
        return np.array(data).reshape(-1, len_row)
    return np.array(data).reshape((-1, len_row, nvars))

def normalize_and_save(X):
    ndim = X.shape[2]
    avg_list = []
    std_list = []
    print('  normalizing train sample')
    for i in range(ndim):
        avg_train = np.average(X[:,:,i].flatten())
        std_train = np.std(X[:,:,i].flatten())
        avg_list.append(avg_train)
        std_list.append(std_train)

        X[:,:,i] -= avg_train
        if std_train != 0:
            X[:,:,i] /= std_train
    norm_const = {'avg_list':avg_list, 'std_list':std_list}
    pickle.dump(norm_const, open('norm_const.p', 'wb'))
    return

def normalize_from_file(X):
    print('  normalizing oos sample')
    norm_const = pickle.load(open('norm_const.p', 'rb'))
    avg_list = norm_const['avg_list']
    std_list = norm_const['std_list']
    for i, (avg_train, std_train) in enumerate(zip(avg_list, std_list)):
        X[:,:,i] -= avg_train
        if std_train != 0:
            X[:,:,i] /= std_train
    return

def print_avg_std(X):
    ndim = X.shape[2]
    print('    {} {:>8s} {:>8s}'.format('i', 'avg', 'std'))
    for i in range(ndim):
        avg_X = np.average(X[:,:,i].flatten())
        std_X = np.std(X[:,:,i].flatten())
        print('    {} {:8.4f} {:8.4f}'.format(i, avg_X, std_X))

def csv2numpy(dfrom, dto, input_type, label_type, sample_type, nvars=1, normalize=False, volcut=None):
    xnames = get_filenames(dfrom, dto, input_type)
    idates, tickers, timestamps = get_meta(xnames)
    X = iter_loadtxt(xnames, skiprows=0, dtype=np.float32, nvars=nvars)
    if volcut is not None:
        nInputTickers = X.shape[1]
        nSelectTickers = int(nInputTickers * volcut)
        X = X[:,-nSelectTickers:,:]
        print("  volcut={}, X shape changed:{}\n".format(volcut, X.shape))
    print('read X with shape:', X.shape)

    # Normalize X
    print_avg_std(X)
    if normalize:
        if sample_type == 'train':
            normalize_and_save(X)
        else:
            normalize_from_file(X)
    print_avg_std(X)
    
    ynames = get_filenames(dfrom, dto, label_type)
    Y = iter_loadtxt(ynames, skiprows=0, dtype=np.float32)
    print('read Y with shape:', Y.shape)
    
    wnames = get_filenames(dfrom, dto, 'wgt')
    W = iter_loadtxt(wnames, skiprows=0, dtype=np.float32)
    print('read W with shape:', W.shape)
    
    output_name = '{}.npz'.format(sample_type)
    print('writing npz file: {}\n'.format(output_name))
    np.savez_compressed(output_name, X=X, Y=Y, W=W, idates=idates, tickers=tickers, timestamps=timestamps)

def csv2numpyTwoInput(dfrom, dto, input_type, label_type, sample_type, nvars, normalize=False):
    '''Combine two input files (such as 5input + predsig)
    '''
    X = [None] * len(input_type)
    for i, itype in enumerate(input_type):
        xnames = get_filenames(dfrom, dto, itype)
        X[i] = loadtxt(xnames, skiprows=0, dtype=np.float32, nvars=nvars[i])
    X = np.dstack(X)
    print('read X with shape:', X.shape)

    # Normalize X
    print_avg_std(X)
    if normalize:
        if sample_type == 'train':
            normalize_and_save(X)
        else:
            normalize_from_file(X)
    print_avg_std(X)
    
    ynames = get_filenames(dfrom, dto, label_type)
    Y = iter_loadtxt(ynames, skiprows=0, dtype=np.float32)
    print('read Y with shape:', Y.shape)
    
    output_name = '{}.npz'.format(sample_type)
    print('writing npz file: {}\n'.format(output_name))
    np.savez_compressed(output_name, X=X, Y=Y)
