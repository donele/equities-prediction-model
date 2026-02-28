#!/bin/python

import string
import os
import sys

def getDir(model, fitDesc, sigType, udate):
	desc = ''
	if fitDesc == 'tevt':
		desc = 'Tevt'

	if sigType == 'om':
		#target = 'restar60;0'
                #if model[0] == 'U':
		    target = 'tar60;0'
                #else:
		    #target = 'tar60;0_xbmpred60;0'
	elif sigType == 'tm':
		target = 'tar600;60_1.0_tar3600;660_0.5'

	dir = '/home/jelee/hffit/%s/fit%s/%s/stat_%d' % (model, desc, target, udate)
	return dir

def getOosFiles(dir, bound1, bound2):
	files = os.listdir(dir)
	oosFiles = []
	for f in files:
		if f[0:6] == 'anaoos' and len(f) == 28:
                    d1 = int(f[7:15])
                    d2 = int(f[16:24])
                    if d1 >= bound1 and d2 <= bound2:
			oosFiles.append(f)
	return oosFiles

def getDateTo(filename):
	return int(filename[16:24])

def getDateFrom(filename):
	return int(filename[7:15])

def chooseOosFile(files):
	tmpFiles = []
	maxDateTo = 0
	for f in files:
		dateTo = getDateTo(f)
		if dateTo > maxDateTo:
			maxDateTo = dateTo
			tmpFiles.append(f)

	minDateFrom = 99999999
	ret = ''
	for f in tmpFiles:
		dateFrom = getDateFrom(f)
		if dateFrom < minDateFrom:
			minDateFrom = dateFrom
			ret = f

	return ret

def getOosFile(dir1, dir2, bound1, bound2):
	files1 = getOosFiles(dir1, bound1, bound2)
	files2 = getOosFiles(dir2, bound1, bound2)
	commonFiles = []
	for f1 in files1:
		if f1 in files2:
			commonFiles.append(f1)
	filename = chooseOosFile(commonFiles)
	return filename

def getNTree(path):
	maxNTree = 0
	if os.path.isfile(path):
		f = open(path, 'r')
		indx = -1
		for line in f:
			sl = string.split(line)
			if line[0:4] == 'sprd':
				indx = sl.index('nTree')
			elif line[0:4] == 'mnSp':
				indx = sl.index('ntree')
			else:
				nTree = int(sl[indx].strip())
				if nTree > maxNTree:
					maxNTree = nTree
	return maxNTree

def getNTreeCommon(dir1, dir2, oosFile):
	path1 = dir1 + '/' + oosFile
	path2 = dir2 + '/' + oosFile
	nTree = min(getNTree(path1), getNTree(path2))
	return nTree

def printSumm(d, oosFile, nTree):
	path = d + '/' + oosFile
	f = open(path, 'r')
	indx = -1
	for line in f:
		sl = string.split(line)
		if line[0:4] == 'sprd':
			indx = sl.index('nTree')
		elif line[0:4] == 'mnSp':
			indx = sl.index('ntree')
		else:
			lineNTree = int(sl[indx])
			if lineNTree == nTree:
				print line,
	return

def compModels(model, fitDesc, sigType, udate1, udate2, bound1, bound2):
	dir1 = getDir(model, fitDesc, sigType, udate1)
	dir2 = getDir(model, fitDesc, sigType, udate2)
	oosFile = getOosFile(dir1, dir2, bound1, bound2)
	if oosFile != '':
		nTree = getNTreeCommon(dir1, dir2, oosFile)
		dateFrom = getDateFrom(oosFile)
		dateTo = getDateTo(oosFile)
		if nTree > 0:
			print '%s %s OOS: %d-%d' % (fitDesc, sigType, dateFrom, dateTo)
			print 'udate: %d' % udate1
			printSumm(dir1, oosFile, nTree)
			print 'udate: %d' % udate2
			printSumm(dir2, oosFile, nTree)
		else:
			print 'Error finding nTree.'
	else:
		print 'Error finding oos file. %s %s %s %d %d' % (model, fitDesc, sigType, udate1, udate2)

def getFitDescs(model):
	modelBase = model[0:2]
	if modelBase in ['US', 'UF', 'CA', 'EU']:
		return ['reg', 'tevt']
	else:
		return ['reg']
	return []

def getSigTypes(fitDesc):
	if fitDesc == 'reg':
		return ['om', 'tm']
	else:
		return ['om']
	return []

def getAllModels():
	#return ['US', 'UF', 'CA', 'EU', 'AT', 'AH', 'AS', 'KR', 'MJ', 'SS', 'AA']
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

def validDate(sdate):
	idate = int(sdate)
	return idate > 20000000 and idate < 21000000

models = getModels(sys.argv)
udateBounds = [[20160115, 20160215], [20160415, 20160515], [20160715, 20160815], [20161001, 20161101]]
udatePairs = [[20151001, 20160101], [20160101, 20160401], [20160401, 20160701], [20160701, 20160915]]
for m in models:
    print
    print m
    fitDescs = getFitDescs(m)
    for fd in fitDescs:
        sigTypes = getSigTypes(fd)
        for st in sigTypes:
            for i in range(len(udatePairs)):
                udate1 = udatePairs[i][0]
                udate2 = udatePairs[i][1]
                bound1 = udateBounds[i][0]
                bound2 = udateBounds[i][1]
                compModels(m, fd, st, udate1, udate2, bound1, bound2)
