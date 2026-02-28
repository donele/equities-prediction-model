#!/bin/python

import string
import os
import sys

def getFitDir(model, fitDesc, sigType):
	desc = ''
	if fitDesc == 'tevt':
		desc = 'Tevt'

	if sigType == 'om':
		target = 'tar60;0'
	elif sigType == 'tm':
		target = 'tar600;60_1.0_tar3600;660_0.5'
	elif sigType == 'omtm':
		target = 'tar60;0_tar600;60_1.0_tar3600;660_0.5'
	elif sigType == 'tc':
		target = 'tar71Close'
	elif sigType == 'tmtc':
		target = 'tar600;60_1.0_tar3600;660_0.5_tar71Close_0.5'
	elif sigType == 'omtmtc':
		target = 'tar60;0_tar600;60_1.0_tar3600;660_0.5_tar71Close_0.5'

	dir = '/home/jelee/hffit/%s/fit%s/%s' % (model, desc, target)
	return dir

def getStatDir(model, fitDesc, sigType, udate):
        fitDir = getFitDir(model, fitDesc, sigType)
	dir = '%s/stat_%d' % (fitDir, udate)
	return dir

def getCoefDir(model, fitDesc, sigType):
        fitDir = getFitDir(model, fitDesc, sigType[:2])
	dir = '%s/coef' % (fitDir)
	return dir

def getOosFiles(dir):
        if not os.path.isdir(dir):
            print('{} not exist'.format(dir))
            return None
	files = os.listdir(dir)
	oosFiles = []
	for f in files:
            if f[0:8] == 'anaNTree':
			oosFiles.append(f)
	return oosFiles

def getDates(filename):
    if filename is not None:
        fromLoc = filename.index('_20')
        dateFrom = int(filename[(fromLoc+1):(fromLoc+9)])
        toLoc = filename.index('_20', fromLoc + 1)
        dateTo = int(filename[(toLoc+1):(toLoc+9)])
        return dateFrom, dateTo
    return 0, 0

def chooseOosFile(files):
        ret = ''
        maxDiff = 0
        for f in files:
		dateFrom, dateTo = getDates(f)
                diffDate = dateTo - dateFrom
                if diffDate > maxDiff:
                    ret = f
                    maxDiff = diffDate
	return ret

def getOosFile(dir1, dir2):
	files1 = getOosFiles(dir1)
	files2 = getOosFiles(dir2)
        if files1 is not None and files2 is not None:
	    commonFiles = []
	    for f1 in files1:
		    if f1 in files2:
			    commonFiles.append(f1)
	    filename = chooseOosFile(commonFiles)
	    return filename
        return None

def getNTree(path):
	maxNTree = 0
	if os.path.isfile(path):
		f = open(path, 'r')
                lines = f.readlines()
                maxNTree = int(string.split(lines[-1])[0])
	return maxNTree

def getNTreeCommon(dir1, dir2, oosFile):
        if oosFile is not None:
	    path1 = dir1 + '/' + oosFile
	    path2 = dir2 + '/' + oosFile
	    nTree = min(getNTree(path1), getNTree(path2))
	    return nTree
        return None

def printSumm(d, oosFile, nTree):
	path = d + '/' + oosFile
	f = open(path, 'r')
	indx = -1
	for line in f:
		sl = string.split(line)
                if sl[0] != 'ntree':
		    lineNTree = int(sl[0])
		    if lineNTree == nTree:
			    print line,
	return

def compModels(model, fitDesc, sigType, udate1, udate2):
	dir1 = getStatDir(model, fitDesc, sigType, udate1)
	dir2 = getStatDir(model, fitDesc, sigType, udate2)
	oosFile = getOosFile(dir1, dir2)
	if oosFile != '':
		nTree = getNTreeCommon(dir1, dir2, oosFile)
		dateFrom, dateTo = getDates(oosFile)
		if dateFrom > 0 and dateTo > 0 and nTree > 0:
			print '%s %s OOS: %d-%d' % (fitDesc, sigType, dateFrom, dateTo)
			print '[udate: %d]' % udate1,
			printSumm(dir1, oosFile, nTree)
			print '[udate: %d]' % udate2,
			printSumm(dir2, oosFile, nTree)
		else:
			print 'Error finding nTree.'
	else:
		print 'Error finding oos file. %s %s %s %d %d' % (model, fitDesc, sigType, udate1, udate2)

def getFitDescs(model):
	modelBase = model[0:2]
        if modelBase in ['US', 'UF', 'CA', 'EU', 'AT', 'AS']:
		return ['reg', 'tevt']
	else:
		return ['reg']
	return []

def getSigTypes(model, fitDesc):
	if fitDesc == 'reg':
		return ['om', 'tm', 'tc', 'omtm', 'tmtc', 'omtmtc']
	else:
		return ['om']
	return []

def getAllModels():
	return ['US', 'UF', 'CA', 'EU', 'AT', 'AH', 'AS', 'KR', 'AA']

def getModels(argv):
	argn = len(argv)
	if argn == 1 or argn == 3:
		return getAllModels()
	elif argn > 1:
		model = argv[1]
		modelBase = model[0:2]
		if modelBase in getAllModels():
			return [model]
	return []

def getUdatesFromDisk(model, fitDesc, sigType):
        cdir = getCoefDir(model, fitDesc, sigType)
        files = os.listdir(cdir)
        dates = []
        for f in files:
            if len(f) == 16:
                date = int(f[4:12])
                dates.append(date)
        dates.sort()
        return dates[-2:]

def validDate(sdate):
	idate = int(sdate)
	return idate > 20000000 and idate < 21000000

def getUdates(argv):
	argn = len(argv)
	if argn == 3:
		if validDate(argv[1]) and validDate(argv[2]):
			return [int(argv[1]), int(argv[2])]
	elif argn == 4:
		if validDate(argv[2]) and validDate(argv[3]):
			return [int(argv[2]), int(argv[3])]
	return []

models = getModels(sys.argv)
udatesArg = getUdates(sys.argv)
for m in models:
	print
	print m
	fitDescs = getFitDescs(m)
	for fd in fitDescs:
		sigTypes = getSigTypes(m, fd)
		for st in sigTypes:
			if len(udatesArg) == 2:
				udates = udatesArg
			else:
				udates = getUdatesFromDisk(m, fd, st)
			compModels(m, fd, st, udates[0], udates[1])
