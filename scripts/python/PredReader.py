#!python
import sys
import glob
import collections
import numpy as np
from struct import *
import MFtn
import pandas as pd

class PredReader:
    def __init__(self, market, readDates):
        self.market = market
        self.t = None
        self.y = None
        self.sprd = None
        if not isinstance(readDates, collections.Iterable):
            readDates = [readDates]
        self.nDays = len(readDates)
        self.read(readDates)

    @classmethod
    def forOOS(cls, market, udate, dateFrom, dateTo):
        readDates = MFtn.getOOSDates(MFtn.fitDir(market), udate, dateFrom, dateTo)
        return cls(market, readDates)

    def read(self, readDates):
        for idate in readDates:
            if self.t is None:
                self.t, self.y, self.sprd = self.readDay(idate)
            else:
                t, y, sprd = self.readDay(idate)
                self.t = np.concatenate((self.t, t), axis=0)
                self.y = np.concatenate((self.y, y), axis=0)
                self.sprd = np.concatenate((self.sprd, sprd), axis=0)

    def readDay(self, idate, debug=False, maxRow=None):
        predPath = '{}/fit/tar600;60_1.0_tar3600;660_0.5/stat_20190401/preds/pred{}.txt'.format(MFtn.fitDir(self.market), idate)
        df = pd.read_csv(predPath, delim_whitespace=True)
        N = df.shape[0]
        t = df['target'].to_numpy().reshape(N, 1)
        y = df['pred'].to_numpy().reshape(N, 1)
        sprd = df['sprd'].to_numpy().reshape(N,)

        print(idate, t.shape, y.shape, sprd.shape)
        return t, y, sprd

    def writeWithOtherPred(self, otherPred, outPath):
        return
