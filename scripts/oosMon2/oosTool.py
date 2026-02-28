import numpy as np
import datetime as dt
import os

def getWorkingDir():
    return '.'
    #homeDir = os.path.expanduser('~')
    #wDir = '%s/.oosMon' % homeDir
    #if not os.path.isdir(wDir):
        #os.mkdir(wDir)
    #return wDir

def getHtmlDir():
    htmlDir = '%s/html_oos' % getWorkingDir()
    if not os.path.isdir(htmlDir):
        os.mkdir(htmlDir)
    return htmlDir

def getImgFile(target, var):
    return target + '_' + var + '.png'

def getImgFilePath(target, var):
    htmlDir = getHtmlDir()
    return htmlDir + '/' + getImgFile(target, var)

def getImgFileByNtree(target, var, model):
    return target + '_' + var + '_' + model + '_byNtree.png'

def getImgFileByNtreePath(target, var, model):
    htmlDir = getHtmlDir()
    return htmlDir + '/' + getImgFileByNtree(target, var, model)

def getHtmlMain():
    return '%s/oosSumm.html' % getWorkingDir()

def getHtmlByNtree(target, var):
    return getHtmlDir() + '/' + target + '_' + var + '_ByNtree.html'

def getModels(target):
    allModels = ['US', 'UF', 'CA', 'EU', 'AT', 'AH', 'AS', 'AA', 'KR']
    evtModels = ['US', 'UF', 'CA', 'EU']
    if target == 'ome':
        return evtModels
    return allModels

def getTargets():
    return ['om', 'tm', 'ome']

def getVarList():
    varList = ['mevtb', 'biastb']
    return varList

def getFirstDay():
    d = dt.timedelta(days = 365*2)
    lastDay = getLastDay()
    firstDay = (lastDay - d).replace(day = 1)
    return firstDay

def getLastDay():
    d = dt.timedelta(days = 40)
    today = dt.date.today()
    endDay = (today - d).replace(day = 1)
    return endDay

def getFirstDayOfNextMonth(day):
    firstDay = day.replace(day=1)
    if firstDay.month == 12:
        return firstDay.replace(year = firstDay.year + 1).replace(month = 1)
    return firstDay.replace(month = firstDay.month + 1)

def getYYYYMMs():
    day = getFirstDay()
    lastDay = getLastDay()
    yyyymms = []
    while day <= lastDay:
        yyyymms.append(day.year * 100 + day.month)
        day = getFirstDayOfNextMonth(day)
    return yyyymms

