#!python
import sys
import glob
import collections
import numpy as np
from struct import *
import MFtn
import NpNormalizer

def readSummary(x, y):
    print(x.shape)
    nInputs = x.shape[1]
    print('{:3} {:10} {:10} {:10} {:10} {:10}'.format('i', 'row1', 'mean', 'std', 'min', 'max'))
    for j in range(nInputs):
        print('{:3} {:10.2f} {:10.2f} {:10.2f} {:10.2f} {:10.2f}'.format(j, x[0][j], x[:,j].mean(), x[:,j].std(), x[:,j].min(), x[:,j].max()))
    print('target {:10.2f} {:10.2f} {:10.2f} {:10.2f} {:10.2f}'.format(y[0][0], y[:,0].mean(), y[:,0].std(), y[:,0].min(), y[:,0].max()))

def getRetSeriesThreeRow(vMid, vQI, vSprd, vGptOK, iP):
    lenSeries = [1, 1, 2, 6, 10]
    retSize = len(lenSeries)
    retSer = np.zeros(retSize)
    qISer = np.zeros(retSize)
    sprdSer = np.zeros(retSize)

    lowIndx = iP
    highIndx = iP
    for i, l in enumerate(lenSeries):
        serIndx = retSize - i - 1
        lowIndx = min(0, highIndx - l)
        rtn = 0.
        if vMid[lowIndx] > 1e-2 and vMid[highIndx] > 1e-2 and vGptOK[lowIndx] == 1 and vGptOK[highIndx] == 1:
            rtn = (vMid[highIndx] - vMid[lowIndx]) / vMid[lowIndx] * 10000.
            retSer[serIndx] = rtn;
        if vMid[highIndx] > 1e-2 and vGptOK[highIndx] == 1:
            qISer[serIndx] = vQI[highIndx]
            sprdSer[serIndx] = vSprd[highIndx]
        if lowIndx == 0:
            break
        highIndx = lowIndx
    ret = np.concatenate((retSer, qISer, sprdSer), axis=None)
    return ret

def getRetSeriesAggRtn(vMid, vGptOK, iP):
    lenSeries = [1, 1, 2, 6, 10]
    retSize = len(lenSeries)
    ser = np.zeros(retSize)
    lowIndx = iP
    highIndx = iP
    for i, l in enumerate(lenSeries):
        lowIndx = min(0, highIndx - l)
        rtn = 0.
        if vMid[lowIndx] > 1e-2 and vMid[highIndx] > 1e-2 and vGptOK[lowIndx] == 1 and vGptOK[highIndx] == 1:
            rtn = (vMid[highIndx] - vMid[lowIndx]) / vMid[lowIndx] * 10000.
            ser[retSize - i - 1] = rtn;
        if lowIndx == 0:
            break
        highIndx = lowIndx
    return ser

def getRetSeriesSimple(vMid, vGptOK, iP):
    lenOut = 20
    nData = min(lenOut, iP)
    nZeros = max(lenOut - nData, 0)

    retSeries = np.zeros(nData)
    for i in range(nData):
        indx = iP - nData + i
        rtn = 0.
        if vMid[indx] > 1e-2 and vGptOK[indx+1] == 1 and vGptOK[indx] == 1:
            rtn = (vMid[indx+1] - vMid[indx])/vMid[indx] * 10000.
        retSeries[i] = rtn
    ser = np.append([0] * nZeros, retSeries)
    return ser

class NpInfo:
    def __init__(self, name, mean, std):
        self.name = name
        self.mean = mean
        self.std = std

def read(model, sigType, readDates, inputType):
    dataCnt = []
    lowCnt = {}
    lastCnt = 0
    x = None
    y = None
    sprd = None
    for idate in readDates:
        if x is None:
            x, y, sprd = readDay(model, sigType, idate, inputType)
            cnt = x.shape[0]
            dataCnt.append(cnt)
            if cnt <= .5 * lastCnt:
                lowCnt[idate] = cnt
        else:
            new_x, new_y, new_sprd = readDay(model, sigType, idate, inputType)
            cnt = x.shape[0]
            dataCnt.append(cnt)
            if cnt <= .5 * lastCnt:
                lowCnt[idate] = cnt

            x = np.concatenate((x, new_x), axis=0)
            y = np.concatenate((y, new_y), axis=0)
            sprd = np.concatenate((sprd, new_sprd), axis=0)
    a = np.array(dataCnt)
    print('Read Summary: {} days {}-{}, min {}, max {}, mean {:.1f}'.format(len(dataCnt), readDates[0], readDates[-1], a.min(), a.max(), a.mean()))
    for key, value in lowCnt.items():
        print('   {} {}'.format(key, value))
    print('read()', x.shape, y.shape, sprd.shape)
    return x, y, sprd

def readDay(model, sigType, idate, inputType=None, debug=False, maxRow=None):
    x = []
    y = []
    s = []
    sizeX = 0
    sizeY = 0
    sizeS = 0
    path = '{}/binSig/{}/sig{}.npz'.format(MFtn.fitDir(model), sigType, idate)
    with np.load(path) as data:
        nTicker = data['nTicker'][0]
        nPts = data['nPts'][0]
        vLogVolu = data['vLogVolu']
        vLogPrice = data['vLogPrice']
        vMedVolat = data['vMedVolat']
        vvMid = data['vvMid']
        vvTar1m = data['vvTar1m']
        vvGptOK = data['vvGptOK']
        vvTime = data['vvTime']
        vvSprd = data['vvSprd']
        vvQI = data['vvQI']

        if len(x) == 1:
            sizeX = len(x[0])
            sizeY = len(y[0])
            sizeS = len(S[0])
            if sizeX != sizeY:
                print('sizeX {} sizeY {}'.format(sizeX, sizeY))
            if sizeX != sizeS:
                print('sizeX {} sizeS {}'.format(sizeX, sizeS))

        if nTicker != vvMid.shape[0]:
            print('nTicker does not match {} {}'.format(nTicker, vvMid.shape[0]))
            sys.exit(1)
        if nPts != vvMid.shape[1]:
            print('nPts does not match {} {}'.format(nTicker, vvMid.shape[0]))
            sys.exit(1)

        for iT in range(nTicker):
            if vvGptOK[iT].sum() < .5 * nPts:
                continue

            for iP in range(nPts):
                if vvGptOK[iT][iP] == 1:
                    retSer = None
                    if inputType == 'aggRtn':
                        retSer = getRetSeriesAggRtn(vvMid[iT], vvGptOK[iT], iP)
                    elif inputType == 'simple':
                        retSer = getRetSeriesSimple(vvMid[iT], vvGptOK[iT], iP)
                    elif inputType == 'threeRow':
                        retSer = getRetSeriesThreeRow(vvMid[iT], vvQI[iT], vvSprd[iT], vvGptOK[iT], iP)

                    if retSer[0] > 1000 or retSer[0] < -1000:
                        print('vvMid', vvMid[iT][max(0, iP-20):iP])
                        print('retSer', retSer)
                    addInput = [vvTime[iT][iP], vvSprd[iT][iP], vvQI[iT][iP], vLogVolu[iT], vLogPrice[iT], vMedVolat[iT]]

                    x.append(np.append(retSer, addInput))
                    y.append(vvTar1m[iT][iP])
                    s.append(vvSprd[iT][iP])

            newSizeX = len(x)
            newSizeY = len(y)
            newSizeS = len(s)
            if sizeX > 0 and sizeX != newSizeX:
                print('sizeX {} newSizeX {}'.format(sizeX, newSizeX))
            if sizeX > 0 and sizeX != newSizeY:
                print('sizeX {} newSizeX {}'.format(sizeX, newSizeY))
            if sizeX > 0 and sizeX != newSizeS:
                print('sizeX {} newSizeS {}'.format(sizeX, newSizeS))

    nDataPoints = len(y)
    return [np.array(x), np.array(y).reshape(nDataPoints, 1), np.array(s)]

class NpReader:
    def __init__(self, model, readDates, inputType=None, targetFromSec=0, targetToSec=60):
        self.model = model
        self.sigType = 'np'
        self.inputType = inputType
        self.x = None
        self.y = None
        self.sprd = None
        if not isinstance(readDates, collections.Iterable):
            readDates = [readDates]
        self.nDays = len(readDates)
        self.x, self.y, self.sprd = read(self.model, self.sigType, readDates, self.inputType)
        readSummary(self.x, self.y)

    @classmethod
    def tar1mFit(cls, model, udate, nfitdays, inputType=None, norm=False, excludeRange=None):
        readDates = MFtn.getFitDates(MFtn.fitDir(model), 'np', udate, nfitdays, excludeRange)
        ret = cls(model, readDates, inputType)
        if norm:
            NpNormalizer.forFit(ret.x)
            readSummary(ret.x, ret.y)
        return ret

    @classmethod
    def tar1mOOS(cls, model, udate, dateFrom, dateTo, inputType=None, norm=False, excludeRange=None):
        readDates = MFtn.getOOSDates(MFtn.fitDir(model), 'np', udate, dateFrom, dateTo, excludeRange)
        ret = cls(model, readDates, inputType)
        if norm:
            NpNormalizer.forOOS(ret.x)
            readSummary(ret.x, ret.y)
        return ret

#    def read(self, readDates):
#        dataCnt = []
#        lowCnt = {}
#        lastCnt = 0
#        for idate in readDates:
#            if self.x is None:
#                self.x, self.y, self.sprd = self.readDay(idate)
#                cnt = self.x.shape[0]
#                dataCnt.append(cnt)
#                if cnt <= .5 * lastCnt:
#                    lowCnt[idate] = cnt
#            else:
#                x, y, sprd = self.readDay(idate)
#                cnt = x.shape[0]
#                dataCnt.append(cnt)
#                if cnt <= .5 * lastCnt:
#                    lowCnt[idate] = cnt
#
#                self.x = np.concatenate((self.x, x), axis=0)
#                self.y = np.concatenate((self.y, y), axis=0)
#                self.sprd = np.concatenate((self.sprd, sprd), axis=0)
#        a = np.array(dataCnt)
#        print('Read Summary: {} days {}-{}, min {}, max {}, mean {:.1f}'.format(len(dataCnt), readDates[0], readDates[-1], a.min(), a.max(), a.mean()))
#        for key, value in lowCnt.items():
#            print('   {} {}'.format(key, value))
#
#    def getRetSeries(self, vMid, vGptOK, iP):
#        ser = None
#        if self.aggRtn:
#            lenSeries = [1, 1, 2, 6, 10]
#            retSize = len(lenSeries)
#            ser = np.zeros(retSize)
#            lowIndx = iP
#            highIndx = iP
#            for i, l in enumerate(lenSeries):
#                lowIndx = min(0, highIndx - l)
#                rtn = 0.
#                if vMid[lowIndx] > 1e-2 and vMid[highIndx] > 1e-2 and vGptOK[lowIndx] == 1 and vGptOK[highIndx] == 1:
#                    rtn = (vMid[highIndx] - vMid[lowIndx]) / vMid[lowIndx] * 10000.
#                    ser[retSize - i - 1] = rtn;
#                if lowIndx == 0:
#                    break
#                highIndx = lowIndx
#        else:
#            lenOut = 20
#            nData = min(lenOut, iP)
#            nZeros = max(lenOut - nData, 0)
#
#            retSeries = np.zeros(nData)
#            for i in range(nData):
#                indx = iP - nData + i
#                rtn = 0.
#                if vMid[indx] > 1e-2 and vGptOK[indx+1] == 1 and vGptOK[indx] == 1:
#                    rtn = (vMid[indx+1] - vMid[indx])/vMid[indx] * 10000.
#                retSeries[i] = rtn
#            ser = np.append([0] * nZeros, retSeries)
#        return ser
#
#    def readDay(self, idate, debug=False, maxRow=None):
#        x = []
#        y = []
#        s = []
#        sizeX = 0
#        sizeY = 0
#        sizeS = 0
#        path = '{}/binSig/{}/sig{}.npz'.format(MFtn.fitDir(self.model), self.sigType, idate)
#        with np.load(path) as data:
#            nTicker = data['nTicker'][0]
#            nPts = data['nPts'][0]
#            vLogVolu = data['vLogVolu']
#            vLogPrice = data['vLogPrice']
#            vMedVolat = data['vMedVolat']
#            vvMid = data['vvMid']
#            vvTar1m = data['vvTar1m']
#            vvGptOK = data['vvGptOK']
#            vvTime = data['vvTime']
#            vvSprd = data['vvSprd']
#            vvQI = data['vvQI']
#
#            if len(x) == 1:
#                sizeX = len(x[0])
#                sizeY = len(y[0])
#                sizeS = len(S[0])
#                if sizeX != sizeY:
#                    print('sizeX {} sizeY {}'.format(sizeX, sizeY))
#                if sizeX != sizeS:
#                    print('sizeX {} sizeS {}'.format(sizeX, sizeS))
#
#            if nTicker != vvMid.shape[0]:
#                print('nTicker does not match {} {}'.format(nTicker, vvMid.shape[0]))
#                sys.exit(1)
#            if nPts != vvMid.shape[1]:
#                print('nPts does not match {} {}'.format(nTicker, vvMid.shape[0]))
#                sys.exit(1)
#
#            for iT in range(nTicker):
#                if vvGptOK[iT].sum() < .5 * nPts:
#                    continue
#
#                for iP in range(nPts):
#                    if vvGptOK[iT][iP] == 1:
#                        retSer = self.getRetSeries(vvMid[iT], vvGptOK[iT], iP)
#                        if retSer[0] > 1000 or retSer[0] < -1000:
#                            print('vvMid', vvMid[iT][max(0, iP-20):iP])
#                            print('retSer', retSer)
#                        addInput = [vvTime[iT][iP], vvSprd[iT][iP], vvQI[iT][iP], vLogVolu[iT], vLogPrice[iT], vMedVolat[iT]]
#
#                        x.append(np.append(retSer, addInput))
#                        y.append(vvTar1m[iT][iP])
#                        s.append(vvSprd[iT][iP])
#
#                newSizeX = len(x)
#                newSizeY = len(y)
#                newSizeS = len(s)
#                if sizeX > 0 and sizeX != newSizeX:
#                    print('sizeX {} newSizeX {}'.format(sizeX, newSizeX))
#                if sizeX > 0 and sizeX != newSizeY:
#                    print('sizeX {} newSizeX {}'.format(sizeX, newSizeY))
#                if sizeX > 0 and sizeX != newSizeS:
#                    print('sizeX {} newSizeS {}'.format(sizeX, newSizeS))
#
#        nDataPoints = len(y)
#        return [np.array(x), np.array(y).reshape(nDataPoints, 1), np.array(s)]
