#!/bin/python

import string
import os
import sys

def getDir(model, fitDesc, udate):
	desc = ''
	if fitDesc == 'tevt':
		desc = 'Tevt'

	#target = 'tar60;0_xbmpred60;0'
	target = 'tar60;0'
	dir = '/home/jelee/hffit/%s/fit%s/%s/stat_%d' % (model, desc, target, udate)
	return dir

def getDirNoLin(model, fitDesc, udate):
	desc = ''
	if fitDesc == 'tevt':
		desc = 'Tevt'

	target = 'tar60;0'
	dir = '/home/jelee/hffit/%s/fit%s/%s/stat_%d' % (model, desc, target, udate)
	return dir

def getOosFiles(dir):
	files = os.listdir(dir)
	oosFiles = []
	for f in files:
		if f[0:6] == 'anaoos' and len(f) == 28:
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

def getOosFile(dir1, dir2):
	files1 = getOosFiles(dir1)
	files2 = getOosFiles(dir2)
	commonFiles = []
	for f1 in files1:
		if f1 in files2:
			commonFiles.append(f1)
	filename = chooseOosFile(commonFiles)
	return filename

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

def compModels(model, fitDesc, udate1):
	dir1 = getDir(model, fitDesc, udate1)
	dir2 = getDirNoLin(model, fitDesc, udate1)
	oosFile = getOosFile(dir1, dir2)
	if oosFile != '':
		nTrees = [30, 60]
		dateFrom = getDateFrom(oosFile)
		dateTo = getDateTo(oosFile)
		print '%s OOS: %d-%d' % (fitDesc, dateFrom, dateTo)
		print 'udate: %d' % udate1
		printSumm(dir1, oosFile, nTrees[0])
		print 'udate: %d' % udate1
		printSumm(dir2, oosFile, nTrees[1])
	else:
		print 'Error finding oos file. %s %s %d' % (model, fitDesc, udate1)

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
	return ['UF', 'CA', 'EU', 'AT', 'AH', 'AS', 'KR', 'AA']

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

def getUdate(argv):
	argn = len(argv)
	if argn == 2:
		if validDate(argv[1]):
			return int(argv[1])
	elif argn == 3:
            if validDate(argv[2]):
			return int(argv[2])
	return 0

models = getModels(sys.argv)
#udate = getUdate(sys.argv)
udate = 20161215
for m in models:
	print
	print m
	fitDescs = getFitDescs(m)
	for fd in fitDescs:
		compModels(m, fd, udate)
