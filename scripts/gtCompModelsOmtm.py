#!/bin/python

import string
import os
import sys

class Summary:
    corr = 0.
    mev = 0.
    plPpt = 0.

def getDir(model, udate):
        target = 'tar60;0_tar600;60_1.0_tar3600;660_0.5'
	#target = 'tar60;0_xbmpred60;0_tar600;60_1.0_tar3600;660_0.5'
        #if model[0] == 'U':
	    #target = 'tar60;0_tar600;60_1.0_tar3600;660_0.5'

	dir = '/home/jelee/hffit/%s/fit/%s/stat_%d' % (model, target, udate)
	return dir

def getOosFiles(dir, bound1, bound2):
	files = os.listdir(dir)
	oosFiles = []
	for f in files:
		if f[0:15] == 'anaNTreeMultTar' and len(f) == 37:
                    d1 = int(f[16:24])
                    d2 = int(f[25:33])
                    if d1 >= bound1 and d2 <= bound2:
			oosFiles.append(f)
	return oosFiles

def getDateTo(filename):
	return int(filename[25:33])

def getDateFrom(filename):
	return int(filename[16:24])

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
                        indx = 0
                        if line[0:7] != '  ntree':
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
        theLine = ''
	for line in f:
		sl = string.split(line)
                indx = 0
                if line[0:7] != '  ntree':
			lineNTree = int(sl[indx])
			if lineNTree == nTree:
				#print line,
                                theLine = line
	return theLine

def compModels(model, udate1, udate2, bound1, bound2):
	dir1 = getDir(model, udate1)
	dir2 = getDir(model, udate2)
	oosFile = getOosFile(dir1, dir2, bound1, bound2)
        twoLines = []
	if oosFile != '':
		nTree = getNTreeCommon(dir1, dir2, oosFile)
		dateFrom = getDateFrom(oosFile)
		dateTo = getDateTo(oosFile)
		if nTree > 0:
			#print 'omtm OOS: %d-%d' % (dateFrom, dateTo)
			#print 'udate: %d' % udate1
			line1 = printSumm(dir1, oosFile, nTree)
			#print 'udate: %d' % udate2
			line2 = printSumm(dir2, oosFile, nTree)
                        twoLines = [line1, line2]
		else:
			print 'Error finding nTree.'
	else:
		print 'Error finding oos file. %s %d %d' % (model, udate1, udate2)
        return twoLines

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

def printTwoLines(lines, m, udate1, udate2):
    if len(lines) == 2:
        print '%s %d %d' % (m, udate1, udate2),

        sl1 = string.split(lines[0])
        corr1 = float(sl1[4])
        mev1m1 = float(sl1[7])
        mev40m1 = float(sl1[9])
        tradable1 = float(sl1[13])
        mev1 = float(sl1[14])
        of1 = float(sl1[15])
        plPpt1 = float(sl1[16])
        print '%6.4f %5.2f %5.2f %6.4f %5.2f %5.2f %.4e' % (corr1, mev1m1, mev40m1, tradable1, mev1, of1, plPpt1),

        sl2 = string.split(lines[1])
        corr2 = float(sl2[4])
        mev1m2 = float(sl2[7])
        mev40m2 = float(sl2[9])
        tradable2 = float(sl2[13])
        mev2 = float(sl2[14])
        of2 = float(sl2[15])
        plPpt2 = float(sl2[16])
        print ' %6.4f %5.2f %5.2f %6.4f %5.2f %5.2f %.4e' % (corr2, mev1m2, mev40m2, tradable2, mev2, of2, plPpt2)

def getAllModels():
	#return ['US', 'UF', 'CA', 'EU', 'AT', 'AH', 'AS', 'KR', 'AA']
	return ['US', 'UF', 'CA', 'EU', 'AT', 'AH', 'AS', 'KR']
	#return ['US']

models = getModels(sys.argv)

#udatePairs = [[20151001, 20160101], [20160101, 20160401], [20160401, 20160701], [20160701, 20160915]]
#udateBounds = [[20160115, 20160215], [20160415, 20160515], [20160715, 20160815], [20161001, 20161101]]

#udatePairs = [[20160101, 20160401], [20160401, 20160701], [20160701, 20160915]]
#udateBounds = [[20160415, 20160515], [20160715, 20160815], [20161001, 20161101]]

#udatePairs = [[20141027, 20150101], [20150101, 20150501], [20150501, 20150815], [20150815, 20151001], [20151001, 20160101]]
#udateBounds = [[20150115, 20150215], [20150515, 20150615], [20150901, 20151001], [20151015, 20151115], [20160115, 20160215]]
#udatePairs = [[20150501, 20150815], [20150815, 20151001], [20151001, 20160101]]
#udateBounds = [[20150901, 20151001], [20151015, 20151115], [20160115, 20160215]]
#udatePairs = [[20150815, 20151001], [20151001, 20160101]]
#udateBounds = [[20151015, 20151115], [20160115, 20160215]]

udatePairs = [[20160101, 20160201], [20160101, 20160301], [20160401, 20160401], [20160401, 20160501], [20160401, 20160601], [20160701, 20160701], [20160701, 20160801]]
udateBounds = [[20160215, 20160315],[20160315, 20160415],[20160415, 20160515],[20160515, 20160615],[20160615, 20160715],[20160715, 20160815],[20160815, 20160915]]

for m in models:
    for i in range(len(udatePairs)):
        udate1 = udatePairs[i][0]
        udate2 = udatePairs[i][1]
        bound1 = udateBounds[i][0]
        bound2 = udateBounds[i][1]
        twoLines = compModels(m, udate1, udate2, bound1, bound2)
        printTwoLines(twoLines, m, udate1, udate2)
