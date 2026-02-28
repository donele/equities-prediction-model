import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import os
from oosTool import getModels, getWorkingDir, getTargets, getVarList, getYYYYMMs

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

def getDFMkt(model, target):
    dfs = []
    yyyymms = getYYYYMMs()
    for i, yyyymm in enumerate(yyyymms):
        path = getPath(model, target, yyyymm)
        if path is not None and os.stat(path).st_size > 10:
            df = pd.read_csv(path, delim_whitespace=True)
            df = df[(df['mnSp']==0) & (df['mxSp']==100)]
            df['target'] = target
            df['model'] = model
            df['mo'] = yyyymm
            df['imo'] = i
            df.set_index(['imo'])
            dfs.append(df)
    df = pd.concat(dfs)
    return df

def getDFAllMkt():
    dfs = []
    targets = getTargets()
    for target in targets:
        models = getModels(target)
        for model in models:
            dfs.append(getDFMkt(model, target))
    df = pd.concat(dfs)
    return df

def main():
    df = getDFAllMkt()
    #df = df.reset_index()
    #df = df.set_index(['target','model','mo'])
    #df = df.set_index(['target'])
    df.to_pickle('%s/df.p' % getWorkingDir())

if __name__ == '__main__':
    main()

