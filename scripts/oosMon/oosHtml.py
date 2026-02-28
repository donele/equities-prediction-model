# Avoid X server connection. Should be done before importing pyplot.
import matplotlib
matplotlib.use('Agg')

import numpy as np
import matplotlib.pyplot as plt
import itertools
import pandas as pd
import pickle
import os
from oosTool import getWorkingDir, getTargets, getVarList, getYYYYMMs

def getHtmlDir():
    htmlDir = '{}/html_oos'.format(getWorkingDir())
    if not os.path.isdir(htmlDir):
        os.mkdir(htmlDir)
    return htmlDir

def getHtmlDirRelative():
    htmlDir = 'html_oos'
    if not os.path.isdir(htmlDir):
        os.mkdir(htmlDir)
    return htmlDir

def getImgFile(htmlDir, target, var):
    return htmlDir + '/' + target + '_' + var + '.png'

def makePlot(df, target, var):
    # Figure
    fig = plt.figure(figsize=(8,3))
    ax = fig.add_subplot(1,1,1)

    # Plot.
    df[df['target'] == target].groupby('model')[var].plot(title=target+' '+var, grid=True)
    plt.legend(loc='center left', bbox_to_anchor=(1.0, 0.5))

    # X label.
    x = np.arange(0, len(getYYYYMMs()))
    ax.set_xticks(x)
    ax.set_xticklabels([str(x) for x in getYYYYMMs()], rotation=30, fontsize=7)

    # Save.
    plt.tight_layout()
    plt.subplots_adjust(right=0.9)
    plt.savefig(getImgFile(getHtmlDir(), target, var))

def makePlots(df):
    for target, var in list(itertools.product(getTargets(), getVarList())):
        makePlot(df, target, var)

def makeHtml(df):
    f = open('{}/oosSumm.html'.format(getWorkingDir()), 'w')
    f.write('<html><body>\n')

    for target, var in list(itertools.product(getTargets(), getVarList())):
        f.write('<img src="{}">\n'.format(getImgFile(getHtmlDirRelative(), target, var)))

    f.write('</body></html>\n')

def main():
    dfSumm = pickle.load(open('{}/df.p'.format(getWorkingDir()), 'rb'))
    makePlots(dfSumm)
    makeHtml(dfSumm)

if __name__ == '__main__':
    main()
