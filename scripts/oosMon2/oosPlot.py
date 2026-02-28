# Avoid X server connection. Should be done before importing pyplot.
import matplotlib
matplotlib.use('Agg')

import numpy as np
import matplotlib.pyplot as plt
import itertools
import pandas as pd
import pickle
import oosTool as ot

def makePlot(df, target, var):
    # Figure
    fig = plt.figure(figsize=(8,3))
    ax = fig.add_subplot(1,1,1)

    # Plot.
    #dfTar = df.loc[target]
    dfTar = df[df['target'] == target]
    idxMaxTree = dfTar.groupby(['model','mo'])['ntree'].transform(max) == dfTar['ntree']
    dfTar = dfTar[idxMaxTree]

    dfTar.groupby(['model'])[var].plot(title=target+' '+var,
            grid=True)
    plt.legend(loc='center left', bbox_to_anchor=(1.0, 0.5))

    # X label.
    x = np.arange(0, len(ot.getYYYYMMs()))
    ax.set_xticks(x)
    ax.set_xticklabels([str(x) for x in ot.getYYYYMMs()], rotation=30, fontsize=7)

    # Save.
    plt.tight_layout()
    plt.subplots_adjust(right=0.9)
    plt.savefig(getImgFilePath(target, var))
    plt.close()

def makePlotByNtree(df, target, var, model):
    # Figure
    fig = plt.figure(figsize=(8,3))
    ax = fig.add_subplot(1,1,1)

    # Plot.
    dfTar = df[df['target'] == target]
    dfTar = dfTar[dfTar['model'] == model]
    ntreeList = dfTar[dfTar['mo'] == dfTar['mo'].max()]['ntree'].unique()[-5:]
    dfTar = dfTar[dfTar['ntree'].isin(ntreeList)]
    dfTar.groupby(['ntree'])[var].plot(title=model+' '+target+' '+var+' by nTrees',
            grid=True)
    plt.legend(loc='center left', bbox_to_anchor=(1.0, 0.5))

    # X label.
    x = np.arange(0, len(ot.getYYYYMMs()))
    ax.set_xticks(x)
    ax.set_xticklabels([str(x) for x in ot.getYYYYMMs()], rotation=30, fontsize=7)

    # Save.
    plt.tight_layout()
    plt.subplots_adjust(right=0.9)
    plt.savefig(getImgFileByNtreePath(target, var, model))
    plt.close()

def makePlots(df):
    for target, var in list(itertools.product(ot.getTargets(), ot.getVarList())):
        makePlot(df, target, var)
    for target, var in list(itertools.product(ot.getTargets(), ot.getVarList())):
        models = ot.getModels(target)
        for model in models:
            makePlotByNtree(df, target, var, model)

def main():
    df = pickle.load(open('%s/df.p' % ot.getWorkingDir(), 'rb'))
    df = df.set_index(['imo'])
    makePlots(df)

if __name__ == '__main__':
    main()
