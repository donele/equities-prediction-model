import numpy as np
import datetime as dt
import os

def getWorkingDir():
    homeDir = os.path.expanduser('~')
    wDir = '%s/.oosMon' % homeDir
    if not os.path.isdir(wDir):
        os.mkdir(wDir)
    return wDir

def getTargets():
    return ['om', 'tm', 'ome']

def getVarList():
    varList = ['mev', 'bias']
    return varList

def getFirstDay(nYear=3):
    d = dt.timedelta(days = 365*nYear)
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

