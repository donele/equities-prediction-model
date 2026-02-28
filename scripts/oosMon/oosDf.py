import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import os
from oosTool import getWorkingDir, getTargets, getVarList, getYYYYMMs

def getModels(target):
    allModels = ['US', 'UF', 'CA', 'EU', 'AT', 'AH', 'AS', 'AA', 'KR']
    evtModels = ['US', 'UF', 'CA', 'EU']
    if target == 'ome':
        return evtModels
    return allModels

def getTargetName(model, target, yyyymm):
    name = ''
    if target[0:2] == 'om':
        if model[0] == 'U' and yyyymm >= 201703:
            name = 'tar60;0'
        else:
            name = 'tar60;0_xbmpred60;0'
    else:
        name = 'tar600;60_1.0_tar3600;660_0.5'
    return name

def getCoefDir(model, target):
    return getFitDir(model, target) + '/coef'

def getFitName(target):
    if target == 'ome':
        return 'fitTevt'
    return 'fit'

def getFitDir(model, target, yyyymm):
    baseDir = '/home/jelee/hffit'
    name = getTargetName(model, target, yyyymm)
    fit = getFitName(target)
    fitDir = baseDir + '/' + model + '/' + fit + '/' + name
    return fitDir

def getStatDir(model, target, yyyymm):
    statDir = getFitDir(model, target, yyyymm) + '/stat_0'
    return statDir

def getPath(model, target, yyyymm):
    statDir = getStatDir(model, target, yyyymm)
    filenames = os.listdir(statDir)
    tempname = ''
    min_d1 = 99999999
    max_d2 = 0
    for filename in filenames:
        if len(filename) == 28 and filename[0:6] == 'anaoos':
            d1 = int(filename[7:15])
            d2 = int(filename[16:24])
            if d1 // 100 == yyyymm and d2 // 100 == yyyymm:
                if d1 <= min_d1 and d2 >= max_d2:
                    tempname = filename
                    min_d1 = d1
                    max_d2 = d2
    if tempname == '':
        return None
    return statDir + '/' + tempname

def getRow(model, target, yyyymm):
    path = getPath(model, target, yyyymm)
    row = [target, model, yyyymm, np.nan, np.nan]
    if path is not None and os.stat(path).st_size > 10:
        df = pd.read_csv(path, delim_whitespace=True)
        mev = df.mevtb.iat[-1]
        bias = df.biastb.iat[-1]
        row = [target, model, yyyymm, mev, bias]
    return row

def getDF(model, target):
    summ = []
    for yyyymm in getYYYYMMs():
        summ.append(getRow(model, target, yyyymm))
    dfSumm = pd.DataFrame(summ, columns=['target', 'model', 'mo', 'mev', 'bias'])
    return dfSumm

def getDFSumm():
    dfs = []
    targets = getTargets()
    for target in targets:
        models = getModels(target)
        for model in models:
            dfs.append(getDF(model, target))
    dfSumm = pd.concat(dfs)
    return dfSumm

def main():
    dfSumm = getDFSumm()
    dfSumm.to_pickle('%s/df.p' % getWorkingDir())

if __name__ == '__main__':
    main()

