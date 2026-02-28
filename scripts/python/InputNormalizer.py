#!python
import os
import sys
import glob
import collections
import numpy as np
import MFtn
from struct import *

class InputNormalizer:
    def __init__(self, market, sigType, udate, nfitdays, excludeRange=None):
        pathInputList = MFtn.getPathInputList(sigType)
        self.normFilename = 'norm.txt'
        self.inputMat = None
        if os.path.exists(self.normFilename):
            print('Reading normalization factors from existing file...')
            sys.stdout.flush()
        else:
            print('Reading data for normalization...')
            sys.stdout.flush()
            self.market = market
            readDates = MFtn.getFitDates(MFtn.fitDir(market), sigType, udate, nfitdays, excludeRange)
            self.inputNames = self.getInputList(pathInputList)
            self.nInputs = len(self.inputNames)
            if not isinstance(readDates, collections.Iterable):
                readDates = [readDates]
            for idate in readDates:
                if self.inputMat is None:
                    inputMat = self.readDay(MFtn.fitDir(market), idate, self.inputNames)
                    self.inputMat = inputMat
                else:
                    inputMat = self.readDay(MFtn.fitDir(market), idate, self.inputNames)
                    self.inputMat = np.concatenate((self.inputMat, inputMat), axis=0)
    def __enter__(self):
        return self
    def __exit__(self, exc_type, exc_value, traceback):
        self.inputMat = None

    def getInputList(self, path):
        inputNames = []
        with open(path, 'r') as f:
            lines = f.readlines()
            for line in lines:
                pieces = line.split()
                if len(pieces) == 2 and pieces[0] == 'input':
                    name = pieces[1]
                    inputNames.append(name)
                elif len(pieces) == 1:
                    inputNames.append(pieces[0])
        return inputNames

    def summ(self):
        print(self.inputMat.shape)
        print('{:3} {:20} {:10} {:10} {:10} {:10} {:10}'.format('i', 'name', 'row1', 'mean', 'std', 'min', 'max'))
        for j in range(self.nInputs):
            print('{:3} {:20} {:10.2f} {:10.2f} {:10.2f} {:10.2f} {:10.2f}'.format(j, self.inputNames[j], self.inputMat[0][j], self.inputMat[:,j].mean(), self.inputMat[:,j].std(), self.inputMat[:,j].min(), self.inputMat[:,j].max()))
    
    def normalize(self):
        if self.inputMat is not None:
            print('Calculating normalization factors...')
            sys.stdout.flush()
            validInputs = []
            validInputMean = []
            validInputStd = []
            invalidCol = []
            for ic in range(self.nInputs):
                m = self.inputMat[:,ic].mean()
                s = self.inputMat[:,ic].std()
                if s > 0.:
                    self.inputMat[:,ic] = (self.inputMat[:,ic] - m) / s
                    validInputs.append(self.inputNames[ic])
                    validInputMean.append(m)
                    validInputStd.append(s)
                else:
                    self.inputMat[:,ic] = 0.
            f = open(self.normFilename, 'w')
            for name, m, s in zip(validInputs, validInputMean, validInputStd):
                f.write('{}\t{:.8f}\t{:.8f}\n'.format(name, m, s))

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

    def readDay(self, fitDir, idate, inputNames, debug=False, maxRow=None):
        nInputs = len(inputNames)
        path = '{}/binSig/tm/sig{}.bin'.format(fitDir, idate)
        with open(path, 'rb') as f:
            nrows = unpack('=i', f.read(4))[0]
            ncols = unpack('=i', f.read(4))[0]
            nlabels = unpack('=q', f.read(8))[0]
            labelList = f.read(nlabels).decode("utf-8").strip('\x00').split(',')
            inputLoc = self.getInputLoc(inputNames, labelList)
    
            nrowsRead = nrows if maxRow == None else maxRow
            inputMat = np.zeros((nrowsRead, nInputs), dtype=float)
            for i in range(nrowsRead):
                arow = unpack('='+str(ncols)+'f', f.read(ncols*4))
                inputs = self.getInputs(arow, inputLoc)
                inputMat[i] = inputs
                if debug and i == 0:
                    for j in range(ncols):
                        print('{:3} {:18s} {:10.2f} {:10} {:10.2f}'.format(j, labelList[j], arow[j], (inputLoc[j] if j<nInputs else ''), (inputs[j] if j<nInputs else -999.)))
            print(idate, inputMat.shape)
            return inputMat
        return None
