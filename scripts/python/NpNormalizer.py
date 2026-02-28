#!python
import os
import sys
import glob
import collections
import numpy as np
import MFtn
from struct import *

def forFit(x):
    normFilename = 'norm.txt'
    if os.path.exists(normFilename):
        normalizeFromFile(x, normFilename)
    else:
        calculateNormalization(x, normFilename)
        normalizeFromFile(x, normFilename)

def forOOS(x):
    normFilename = 'norm.txt'
    if os.path.exists(normFilename):
        normalizeFromFile(x, normFilename)
    else:
        print('Normalization information not found.')
        sys.exit(12)

def normalizeFromFile(x, normFilename):
    print('Reading normalization factors from existing file...')
    inputInfoList = []
    with open(normFilename, 'r') as f:
        lines = f.readlines()
        for line in lines:
            pieces = line.split()
            if len(pieces) == 3:
                name = pieces[0]
                mean = float(pieces[1])
                std = float(pieces[2])
                inputInfo = MFtn.InputInfo(name, mean, std)
                inputInfoList.append(inputInfo)

    if x.shape[1] != len(inputInfoList):
        print('x.shap[1] {} len(inputInfoList)'.format(x.shape[1], len(inputInfoList)))
        sys.exit(14)
    for ic, info in enumerate(inputInfoList):
        if info.std > 0.:
            x[:,ic] = (x[:,ic] - info.mean) / info.std
        else:
            x[:,ic] = 0.

def calculateNormalization(x, normFilename):
    print(x.shape)
    nInputs = x.shape[1]
    print('Calculating normalization factors...')
    sys.stdout.flush()

    validIndex = []
    validInputMean = []
    validInputStd = []
    for ic in range(nInputs):
        m = x[:,ic].mean()
        s = x[:,ic].std()
        if s > 0.:
            validIndex.append(ic)
            validInputMean.append(m)
            validInputStd.append(s)
        else:
            validIndex.append(ic)
            validInputMean.append(0.)
            validInputStd.append(0.)

    f = open(normFilename, 'w')
    for name, m, s in zip(validIndex, validInputMean, validInputStd):
        f.write('{}\t{:.8f}\t{:.8f}\n'.format(name, m, s))

