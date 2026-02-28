#!python
import sys
import glob
import collections
import numpy as np
from struct import *
import MFtn

class InputReader:
    def __init__(self, market, sigType, readDates):
        self.market = market
        self.sigType = sigType
        if sigType == 'tm':
            self.targetNames = ['tar600;60', 'tar3600;660']
            self.targetWgts = [1., .5]
            pathInputList='/home/jelee/scripts/gtConf/input_tm.conf'
        elif sigType == 'om':
            self.targetNames = ['tar60;0']
            self.targetWgts = [1.]
            pathInputList='/home/jelee/scripts/gtConf/input_om.conf'
        elif sigType == 'tc':
            self.targetNames = ['tar71Close']
            self.targetWgts = [1.]
        self.x = None
        self.y = None
        self.sprd = None
        if not isinstance(readDates, collections.Iterable):
            readDates = [readDates]
        self.nDays = len(readDates)

        self.inputNames = []
        with open(pathInputList, 'r') as f:
            for line in f:
                pieces = line.split()
                if len(pieces) == 2 and pieces[0] == 'input':
                    name = pieces[1]
                    self.inputNames.append(name)
        self.nInputs = len(self.inputNames)

        self.read(readDates)

    @classmethod
    def forNNFit(cls, market, sigType, udate, nfitdays, excludeRange=None):
        readDates = MFtn.getFitDates(MFtn.fitDir(market), sigType, udate, nfitdays, excludeRange)
        ret = cls(market, sigType, readDates)
        ret.normalize()
        ret.summ()
        return ret

    @classmethod
    def forNNOOS(cls, market, sigType, udate, dateFrom, dateTo, excludeRange=None):
        readDates = MFtn.getOOSDates(MFtn.fitDir(market), sigType, udate, dateFrom, dateTo, excludeRange)
        ret = cls(market, sigType, readDates)
        ret.normalize()
        ret.summ()
        return ret

    @classmethod
    def forTreeFit(cls, market, sigType, udate, nfitdays, excludeRange=None):
        readDates = MFtn.getFitDates(MFtn.fitDir(market), sigType, udate, nfitdays, excludeRange)
        ret = cls(market, sigType, readDates)
        ret.y = ret.y.reshape(-1)
        return ret

    @classmethod
    def forTreeOOS(cls, market, sigType, udate, dateFrom, dateTo, excludeRange=None):
        readDates = MFtn.getOOSDates(MFtn.fitDir(market), sigType, udate, dateFrom, dateTo, excludeRange)
        ret = cls(market, sigType, readDates)
        ret.y = ret.y.reshape(-1)
        return ret

    @classmethod
    def forDirFit(cls, market, sigType, udate, nfitdays):
        readDates = MFtn.getFitDates(MFtn.fitDir(market), sigType, udate, nfitdays)
        ret = cls(market, sigType, readDates)
        ret.y = ret.y.reshape(-1)
        ret.y = np.where(ret.y > 0, 1, 0)
        return ret

    @classmethod
    def forDirOOS(cls, market, sigType, udate, dateFrom, dateTo):
        readDates = MFtn.getOOSDates(MFtn.fitDir(market), sigType, udate, dateFrom, dateTo)
        ret = cls(market, sigType, readDates)
        ret.y = ret.y.reshape(-1)
        ret.y = np.where(ret.y > 0, 1, 0)
        return ret

    @classmethod
    def forEdgeFit(cls, market, sigType, udate, nfitdays):
        readDates = MFtn.getFitDates(MFtn.fitDir(market), sigType, udate, nfitdays)
        ret = cls(market, sigType, readDates)
        ret.y = ret.y.reshape(-1)
        tempPos = np.where((ret.y > 0) & (ret.y > .5*ret.sprd), 2, 0)
        tempNeg = np.where((ret.y < 0) & (-ret.y > .5*ret.sprd), 1, 0)
        tempZero = np.zeros(ret.y.shape[0])
        temp = tempPos + tempNeg + tempZero
        ret.y = temp
        return ret

    @classmethod
    def forEdgeOOS(cls, market, sigType, udate, dateFrom, dateTo):
        readDates = MFtn.getOOSDates(MFtn.fitDir(market), sigType, udate, dateFrom, dateTo)
        ret = cls(market, sigType, readDates)
        ret.y = ret.y.reshape(-1)
        tempPos = np.where((ret.y > 0) & (ret.y > .5*ret.sprd), 1, 0)
        tempNeg = np.where((ret.y < 0) & (-ret.y > .5*ret.sprd), -1, 0)
        tempZero = np.zeros(ret.y.shape[0])
        temp = tempPos + tempNeg + tempZero
        ret.y = temp
        return ret

    def summ(self):
        print(self.x.shape)
        print('{:3} {:20} {:10} {:10} {:10} {:10} {:10}'.format('i', 'name', 'row1', 'mean', 'std', 'min', 'max'))
        for j in range(self.nInputs):
            print('{:3} {:20} {:10.2f} {:10.2f} {:10.2f} {:10.2f} {:10.2f}'.format(j, self.inputNames[j], self.x[0][j], self.x[:,j].mean(), self.x[:,j].std(), self.x[:,j].min(), self.x[:,j].max()))
        print('{:3} {:20} {:10.2f} {:10.2f} {:10.2f} {:10.2f} {:10.2f}'.format('0', 'target', self.y[0][0], self.y[:,0].mean(), self.y[:,0].std(), self.y[:,0].min(), self.y[:,0].max()))
    
    def read(self, readDates):
        dataCnt = []
        lowCnt = {}
        lastCnt = 0
        for idate in readDates:
            if self.x is None:
                self.x, self.y, self.sprd = self.readDay(idate, self.inputNames)
                cnt = self.x.shape[0]
                dataCnt.append(cnt)
                if cnt <= .5 * lastCnt:
                    lowCnt[idate] = cnt
            else:
                x, y, sprd = self.readDay(idate, self.inputNames)
                cnt = x.shape[0]
                dataCnt.append(cnt)
                if cnt <= .5 * lastCnt:
                    lowCnt[idate] = cnt

                self.x = np.concatenate((self.x, x), axis=0)
                self.y = np.concatenate((self.y, y), axis=0)
                self.sprd = np.concatenate((self.sprd, sprd), axis=0)
        a = np.array(dataCnt)
        print('Read Summary: {} days {}-{}, min {}, max {}, mean {:.1f}'.format(len(dataCnt), readDates[0], readDates[-1], a.min(), a.max(), a.mean()))
        for key, value in lowCnt.items():
            print('   {} {}'.format(key, value))

    def normalize(self):
        inputInfoList = []
        with open(path, 'r') as f:
            lines = f.readlines()
            for line in lines:
                pieces = line.split()
                if len(pieces) == 3:
                    name = pieces[0]
                    mean = float(pieces[1])
                    std = float(pieces[2])
                    inputInfo = MFtn.InputInfo(name, mean, std)
                    inputInfoList.append(inputInfo)

        for ic, info in enumerate(inputInfoList):
            if info.std > 0.:
                self.x[:,ic] = (self.x[:,ic] - info.mean) / info.std
            else:
                sys.exit(1)

    def readDay(self, idate, inputNames, debug=False, maxRow=None):
        nInputs = len(inputNames)
        path = '{}/binSig/{}/sig{}.bin'.format(MFtn.fitDir(self.market), self.sigType, idate)
        with open(path, 'rb') as f:
            nrows = unpack('=i', f.read(4))[0]
            ncols = unpack('=i', f.read(4))[0]
            nlabels = unpack('=q', f.read(8))[0]
            labelList = f.read(nlabels).decode("utf-8").strip('\x00').split(',')
            inputLoc = self.getInputLoc(inputNames, labelList)
            targetLoc = self.getTargetLoc(labelList)
            sprdLoc = self.getInputLoc(['sprd'], labelList)
    
            nrowsRead = nrows if maxRow == None else maxRow
            x = np.zeros((nrowsRead, nInputs), dtype=float)
            y = np.zeros((nrowsRead, 1), dtype=float)
            sprd = np.zeros(nrowsRead, dtype=float)
            for i in range(nrowsRead):
                arow = unpack('='+str(ncols)+'f', f.read(ncols*4))

                x[i] = self.getInputs(arow, inputLoc)
                y[i] = self.getTarget(arow, targetLoc)
                sprd[i] = self.getInputs(arow, sprdLoc)[0]

                if debug and i == 0:
                    for j in range(ncols):
                        print('{:3} {:18s} {:10.2f} {:10} {:10.2f}'.format(j, labelList[j], arow[j], (inputLoc[j] if j<nInputs else ''), (inputs[j] if j<nInputs else -999.)))
            if x.shape[0] != y.shape[0] or x.shape[0] != sprd.shape[0]:
                print('ERROR: x: ', x.shape, ' y: ', y.shape, ' sprd: ', sprd.shape)
            return x, y, sprd
        return None

    def getInputLoc(self, inputNames, labelList):
        inputLoc = []
        for inputName in inputNames:
            indx = labelList.index(inputName)
            inputLoc.append(indx)
        return inputLoc
    
    def getInputs(self, row, inputLoc):
        inputs = []
        for indx in inputLoc:
            inputs.append(row[indx])
        return inputs
    
    def getTargetLoc(self, labelList):
        targetLoc = []
        for name in self.targetNames:
            indx = labelList.index(name)
            targetLoc.append(indx)
        return targetLoc
    
    def getTarget(self, row, targetLoc):
        target = 0.
        for indx, wgt in zip(targetLoc, self.targetWgts):
            target += row[indx] * wgt
        return target
