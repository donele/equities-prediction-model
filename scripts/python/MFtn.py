import glob
import math
import sys
import numpy as np
import pandas as pd

class FakeData:
    def __init__(self, nInput, nData):
        self.x = np.random.rand(nData, nInput)
        self.y = np.random.rand(nData, 1)
        self.sprd = np.random.rand(nData)

class InputInfo:
    def __init__(self, name, mean, std):
        self.name = name
        self.mean = mean
        self.std = std

def getPathInputList(sigType):
    path = None
    if sigType == 'om':
        path = '/home/jelee/scripts/model/input_om.conf'
    elif sigType in ['tm', 'tc']:
        path = '/home/jelee/scripts/model/input_tm.conf'
    return path

def fitDir(market):
    return '/home/jelee/hffit/{}/'.format(market)

def validDate(idate):
    return idate > 20000000 and idate < 21000000

def getFitDates(fitDir, sigType, udate, nfitdays, excludeRange):
    ext = 'bin'
    if sigType == 'np':
        ext = 'npz'

    dataDir = '{}/binSig/{}'.format(fitDir, sigType)
    filenames = glob.glob('{}/sig*.{}'.format(dataDir, ext))
    idates = []
    for path in filenames:
        if len(path) >= 15:
            filename = path[-15:]
            idate = int(filename[3:11])
            if validDate(idate) and idate < udate:
                if excludeRange is None or (len(excludeRange) == 2 and validDate(excludeRange[0]) and validDate(excludeRange[1]) and (idate < excludeRange[0] or idate > excludeRange[1])):
                    idates.append(idate)
    idates.sort()
    if len(idates) >= nfitdays:
        return idates[-nfitdays:]
    else:
        print('not enough input files')
    return None

def getOOSDates(fitDir, sigType, udate, oosFrom, oosTo, excludeRange):
    ext = 'bin'
    if sigType == 'np':
        ext = 'npz'

    dataDir = '{}/binSig/{}'.format(fitDir, sigType)
    filenames = glob.glob('{}/sig*.{}'.format(dataDir, ext))
    idates = []
    for path in filenames:
        if len(path) >= 15:
            filename = path[-15:]
            idate = int(filename[3:11])
            if validDate(idate) and idate >= oosFrom and idate <= oosTo:
                idates.append(idate)
    idates.sort()
    return idates

def printTradable(df, nDays):
    mev = (df['target'] * df['psign'] - df['cost']).mean()
    mbias = ((df['pred'] - df['target']) * df['psign']).mean()
    ntrd = df.shape[0]
    ss = df['pred'] * df['target'] > 0
    prof = (df['target'] > .5*df['cost']) | (-df['target'] > .5*df['cost'])
    ntrdss = df[ss].shape[0]
    nprof = df[ss & prof].shape[0]
    if ntrd > 0:
        ntssr = float(ntrdss) / ntrd * 100
        nprofr = float(nprof) / ntrd * 100
    print('mev {:.4f} mbias {:.4f} nt {} ntss {} ntos {} ntss/nt% {:.1f} nprof {} nprof/nt% {:.1f} pl {:.1f} nt/d {:.1f} pl/d {:.1f}'.format(mev, mbias, ntrd, ntrdss, ntrd-ntrdss, ntssr, nprof, nprofr, mev*ntrd, ntrd/nDays, mev*ntrd/nDays))

def printTopBottom(df):
    ev = (df['target'] * df['psign']).mean()
    of = ((df['pred'] - df['target']) * df['psign']).mean()
    print('ev {:.4f} of {:.4f}'.format(ev, of))

def printOOSAna(pred, y, sprd, nDays, thres, fee):
    minSprd = 0
    maxSprd = 100
    df = pd.DataFrame({'pred':pred, 'target':y, 'sprd':sprd})
    df['cost'] = df['sprd'] / 2 + thres + fee
    sprdRange = df[(df['sprd'] < minSprd) | (df['sprd'] > maxSprd)].index
    df.drop(sprdRange, inplace=True)
    df['psign'] = 1
    negpred = df['pred'] < 0
    df.loc[negpred, 'psign'] = -1

    corr = df['pred'].corr(df['target'])
    if math.isnan(corr):
        print(df['pred'])
        print(df['target'])
        sys.exit(11)
    tradable = (df['pred'] > df['cost'] + thres) | (-df['pred'] > df['cost'] + thres)
    q1 = df['pred'].quantile(.01)
    q99 = df['pred'].quantile(.99)
    topbottom = (df['pred'] < q1) | (df['pred'] > q99)

    print('corr {:.4f}'.format(corr))
    printTradable(df[tradable], nDays)
    printTopBottom(df[topbottom])

def getCorr(pred, y, sprd, minSprd, maxSprd):
    df = pd.DataFrame({'pred':pred, 'target':y, 'sprd':sprd})
    sprdRange = df[(df['sprd'] < minSprd) | (df['sprd'] > maxSprd)].index
    df.drop(sprdRange, inplace=True)
    corr = df['pred'].corr(df['target'])
    return corr

