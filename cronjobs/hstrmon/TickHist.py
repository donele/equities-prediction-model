import os
import math
import string
import sys
import datetime
from WebMon import *

#------------------------------------------------------------------------------
# Variables, Exchanges, Views, and Ndays
#------------------------------------------------------------------------------

class Variables:
    def __init__( self ):
        self.variables_ = ['filesize']
        return

    def abbrev( variable ):
        if 'filesize' == variable:
            return 'fs'
        return variable[0:2]
    abbrev = staticmethod(abbrev)

    def fullname( variable ):
        if 'filesize' == variable:
            return 'File size'
        return variable
    fullname = staticmethod(fullname)

    def variables( self ):
        return self.variables_

class Exchanges:

    def __init__( self ):
        self.reg_mkt = {}
        self.reg_mkt['EURO'] = ['EA','EB','EI','EP','EF','EL','ED','EM','EZ','EO','EX','EC','EW','EY','EH','EG','ET']
        self.reg_mkt['ASIA'] = ['AT', 'AH', 'AS', 'AK', 'AQ', 'AW', 'AG']
        self.reg_mkt['CAN'] = ['CJ', 'CL', 'CC', 'CO', 'CP']
        self.reg_mkt['MA'] = ['MJ']
        return

    def city(locale):
        if 'ALL' == locale:
            return 'All the markets'
        elif 'EURO' == locale:
            return 'Europe'
        elif 'ASIA' == locale:
            return 'Asia'
        elif 'CAN' == locale:
            return 'Canada'
        elif 'MA' == locale:
            return 'MidEast&Africa'
        elif 'EA' == locale:
            return 'Amsterdam'
        elif 'EB' == locale:
            return 'Brussels'
        elif 'EI' == locale:
            return 'Lisbon'
        elif 'EP' == locale:
            return 'Paris'
        elif 'EF' == locale:
            return 'Frankfurt'
        elif 'EL' == locale:
            return 'London'
        elif 'ED' == locale:
            return 'Madrid'
        elif 'EM' == locale:
            return 'Milan'
        elif 'EZ' == locale:
            return 'Zurich'
        elif 'EO' == locale:
            return 'Oslo'
        elif 'EX' == locale:
            return 'Virt-X'
        elif 'EC' == locale:
            return 'Copenhagen'
        elif 'EW' == locale:
            return 'Stockholm'
        elif 'EY' == locale:
            return 'Helsinki'
        elif 'EH' == locale:
            return 'EU_Chi-X'
        elif 'AT' == locale:
            return 'Tokyo'
        elif 'AH' == locale:
            return 'Hong Kong'
        elif 'AS' == locale:
            return 'Sydney'
        elif 'AK' == locale:
            return 'KOSPI'
        elif 'AQ' == locale:
            return 'KOSDAQ'
        elif 'AW' == locale:
            return 'Taiwan'
        elif 'AG' == locale:
            return 'Singapore'
        elif 'CJ' == locale:
            return 'Toronto'
        elif 'CL' == locale:
            return 'Montreal'
        elif 'CC' == locale:
            return 'CA_Chi-X'
        elif 'CO' == locale:
            return 'Omega'
        elif 'CP' == locale:
            return 'Pure'
        elif 'MJ' == locale:
            return 'Johannesburg'
        return locale
    city = staticmethod(city)

    def locales( self ):
        ret = ['ALL']
        ret += [ r for r in self.reg_mkt ]
        for r in self.reg_mkt:
            ret += self.reg_mkt[r]
        return ret

    def regions( self ):
        ret = ['ALL']
        ret += [ r for r in self.reg_mkt ]
        return ret

    def markets( self, locale = '' ):
        ret = []
        if '' == locale or 'ALL' == locale:
            for r in self.reg_mkt:
                ret += self.reg_mkt[r]
        elif self.is_region(locale):
            ret = self.reg_mkt[locale]
        else:
            ret.append(locale)
        return ret

    def is_region( self, locale ):
        return "ALL" == locale or "EURO" == locale or "ASIA" == locale or "CAN" == locale or "MA" == locale

class Views:
    def __init__( self, variable ):
        if 'filesize' == variable:
            self.views_ = ['sumAll', 'quoteL1', 'quoteL2', 'tradeL1', 'tradeL2', 'order', 'return']
        self.variable = variable
        return

    def views( self ):
        return self.views_

    def viewsSimple( self ):
        if 'filesize' == self.variable:
            return self.views_[1:]
        return self.views_

    def viewsSum( self ):
        if 'filesize' == self.variable:
            return self.views_[0:1]
        return []

    def abbrev( view ):
        if 'quoteL1' == view:
            ret = 'quL1'
        elif 'quoteL2' == view:
            ret = 'quL2'
        elif 'tradeL1' == view:
            ret = 'trL1'
        elif 'tradeL2' == view:
            ret = 'trL2'
        else:
            ret = view[0:2]
        return ret
    abbrev = staticmethod(abbrev)

    def fullname( view ):
        if 'quoteL1' == view:
            return 'Level-1 Quote'
        elif 'quoteL2' == view:
            return 'Level-2 Quote'
        elif 'tradeL1' == view:
            return 'Level-1 Trade'
        elif 'tradeL2' == view:
            return 'Level-2 Trade'
        elif 'return' == view:
            return '1 Second return'
        elif 'order' == view:
            return 'Orderbook'
        elif 'sumAll' == view:
            return 'All the tick data'
        return view
    fullname = staticmethod(fullname)

class Ndays:
    def __init__ ( self ):
        return

    def ndays():
        return [10, 50, 365]
    ndays = staticmethod(ndays)

#------------------------------------------------------------------------------
# Functions
#------------------------------------------------------------------------------

def get_title( variable, locale, view ):
    title = locale + "_" + Views.abbrev(view) + "_" + Variables.abbrev(variable)
    return title

def get_name_locale( locale, variable, ndays ):
    ret = '%s_%s_%dd' % (locale, Variables.abbrev(variable), ndays)
    return ret

def get_dest_level( variable, locale, view ):
    ret = 0
    if 'filesize' == variable:
        if 'quoteL1' == view or 'tradeL1' == view:
            ret = 1
        elif 'quoteL2' == view or 'tradeL2' == view or 'order' == view:
            ret = 2
        elif 'return' == view:
            if 'CJ' == locale or 'A' == locale[0] or 'E' == locale[0]:
                ret = 2
            else:
                ret = 1
    return ret

def get_filesize( market, view, dest_level, date ):
    if date.weekday() >= 5: # Sat Sun
        return 0.0

#   pathbase = '\\\\smrc-ltc-mrct16\\data00\\tickC\\'
    pathbase = '\\\\smrc-ltc-mrct50\\data00\\tickC\\'
    pathmarket = ''
    pathtail = '%d\\%d%02d\\' % (date.year, date.year, date.month)
    pathfile = ''

    M = string.upper(market[1])

    if 'E' == market[0]:
        if 'EH' == market:
            if 2 == dest_level:
                pathmarket = 'eu\\binHist\\%s\\' % (M)
        else:
            pathmarket = 'eu\\binLogL%s\\%s\\' % (dest_level, M)
    elif 'A' == market[0]:
        pathmarket = 'asia\\binLogL%s\\%s\\' % (dest_level, M)
    elif 'CJ' == market:
        if 1 == dest_level:
            pathmarket = 'ca\\tsx\\binLog\\'
        elif 2 == dest_level:
            pathmarket = 'ca\\tsx\\binLogL2\\'
    elif 'CL' == market:
        if 1 == dest_level:
            pathmarket = 'ca\\mnx\\binLog\\'
    elif 'CC' == market or 'CO' == market or 'CP' == market:
        if 2 == dest_level:
            pathmarket = 'ca\\ecn\\binHist\\%s\\' % M
    elif 'M' == market[0]:
        pathmarket = 'ma\\binLogL%s\\%s\\' % (dest_level, M)

    if len(view) == 7 and 'quote' == view[0:5]:
        pathfile = 'q%d%02d_%d.bin' % (date.year, date.month, date.day)
    elif len(view) == 7 and 'trade' == view[0:5]:
        pathfile = 't%d%02d_%d.bin' % (date.year, date.month, date.day)
    elif 'return' == view:
        pathfile = 're%d%02d%02d.bin' % (date.year, date.month, date.day)
    elif 'order' == view:
        pathfile = 'ob%d%02d%02d.bin' % (date.year, date.month, date.day)

    path = pathbase + pathmarket + pathtail + pathfile
    size = 0.0
    if os.path.exists(path):
        size = os.path.getsize(path)

    return size


#------------------------------------------------------------------------------
# Loopers and Makers
#------------------------------------------------------------------------------

class RRDLooper:
    def __init__ (self):
        self.variables = Variables()
        self.exchanges = Exchanges()
        return

    def titles( self ):
        titles = []
        for variable in self.variables.variables():
            views = Views(variable)
            for locale in self.exchanges.markets() + self.exchanges.regions():
                for view in views.viewsSimple():
                    title = get_title( variable, locale, view )
                    titles.append(title)
                for view in views.viewsSum():
                    title = get_title( variable, locale, view )
                    titles.append(title)
        return titles

    def fullnames( self ):
        fullnames = []
        for variable in self.variables.variables():
            views = Views(variable)
            for locale in self.exchanges.markets() + self.exchanges.regions():
                for view in views.viewsSimple():
                    fullname = self.get_fullname( variable, locale, view )
                    fullnames.append(fullname)
                for view in views.viewsSum():
                    fullname = self.get_fullname( variable, locale, view )
                    fullnames.append(fullname)
        return fullnames

    def varList( self ):
        varList = []
        for variable in self.variables.variables():
            views = Views(variable)
            for locale in self.exchanges.markets() + self.exchanges.regions():
                for view in views.viewsSimple():
                    varList.append(variable)
                for view in views.viewsSum():
                    varList.append(variable)
        return varList

    def values( self, date ):
        values = []
        for variable in self.variables.variables():
            views = Views(variable)
            mVal = {}
            for locale in self.exchanges.markets():
                mVal[locale] = {}
                for view in views.views():
                    mVal[locale][view] = 0.0

            for locale in self.exchanges.markets():
                vsum = 0.0
                for view in views.viewsSimple():
                    val = 0.0
                    if 'filesize' == variable:
                        dest_level = get_dest_level(variable, locale, view)
                        val = get_filesize( locale, view, dest_level, date )
                    values.append(val)
                    mVal[locale][view] = val
                    vsum += val
                for view in views.viewsSum():
                    val = vsum
                    values.append(val)
                    mVal[locale][view] = val
            for region in self.exchanges.regions():
                vsum = 0.0
                for view in views.viewsSimple():
                    val = 0.0
                    for locale in self.exchanges.markets(region):
                        val += mVal[locale][view]
                    values.append(val)
                    vsum += val
                for view in views.viewsSum():
                    val = vsum
                    values.append(val)
        return values

    def get_fullname( self, variable, locale, view ):
        city = Exchanges.city(locale)
        viewname = Views.fullname(view)
        variablename = Variables.fullname(variable)
        fullname = variablename + ' of ' + viewname + ', ' + city
        return fullname


class RRDMaker:
    def __init__( self ):
        self.looper = RRDLooper()
        self.t0 = datetime.datetime(1970, 1, 1, 0);
        return

    def create( self, idate = 0 ):
        step = 86400

        date2 = datetime.datetime.utcnow().date()
        if 0 != idate:
            date2 = self.idate2date( idate )
        date1 = date2 - datetime.timedelta( days = 365 )

        print 'creating %s...' % (date1)
        start = self.get_time(date1)
        cmd = 'rrdtool create tickdata.rrd --start %d --step=%d' % (start, step)

        titles = self.looper.titles()
        for title in titles:
            cmd += ' DS:%s:GAUGE:%d:U:U' % (title, step)

        cmd += ' RRA:AVERAGE:0:1:365'
        os.system(cmd)
        return

    def update( self, idate1 = 0, idate2 = 0 ):
        print 'updating...'
        date2 = datetime.datetime.utcnow().date()
        date1 = date2 - datetime.timedelta( days = 364 )

        if 0 != idate2 and 0 != idate1:
            date2 = self.idate2date( idate2 )
            date1 = self.idate2date( idate1 )

        datelist = self.get_datelist(date1, date2)
        for date in datelist:
            seconds = self.get_time(date, 1)
            cmd = 'rrdtool update tickdata.rrd %d' % seconds

            for value in self.looper.values(date):
                cmd += ':%.0f' % value

            os.system(cmd)
        return

    def graph( self, idate = 0 ):
        pngdir = 'png_filesize'
        if os.path.isdir(pngdir):
            pass
        else:
            os.mkdir(pngdir)

        date2 = datetime.datetime.utcnow().date()
        if 0 != idate:
            date2 = self.idate2date( idate )

        print 'plotting...'
        for n in Ndays.ndays():
            date1 = date2 - datetime.timedelta( days = n )

            start = self.get_time(date1)
            end = self.get_time(date2, 1)
            titles = self.looper.titles()
            fullnames = self.looper.fullnames()
            varList = self.looper.varList()
            for i, title in enumerate(titles):
                cmd1 = 'rrdtool graph png_%s\%s.%d.png --start %d --end %d ' % (varList[i], title, n, start, end)
                #cmd2 = ' --vertical-label byte '
                cmd2 = ''
                cmd3 = ' DEF:%s=tickdata.rrd:%s:AVERAGE LINE2:%s#009900:"%s."' % (title, title, title, fullnames[i])
                cmd = cmd1 + cmd2 + cmd3
                os.system(cmd)

        for variable in self.looper.variables.variables():
            dirname = 'png_%s' % variable
            cmdC1 = 'xcopy /S /Y /I %s \\\\smrc-ltc-mrct50\\data00\\tickMon\\webmon\\%s' % (dirname, dirname)
            cmdD1 = 'rmdir /S /Q %s' % dirname
            os.system(cmdC1)
            os.system(cmdD1)
        cmdD2 = 'del tickdata.rrd'
        os.system(cmdD2)
        return

    def html( self ):
        self.html_main()
        self.html_other()
        self.html_finish()
        return

    def html_main( self ):
        htmldir = 'html_filesize'
        if os.path.isdir(htmldir):
            pass
        else:
            os.mkdir(htmldir)

        fmain = open('hstrTickSumm.html', 'w')
        fmain.write('<html>\n')

        lines = tabMenu('hstrTickSumm', 0)
        for l in lines:
            fmain.write(l)

        fmain.write('<table>\n')

        # Top row
        fmain.write('<tr>')
        fmain.write('<td>Market</td>')
        for variable in self.looper.variables.variables():
            fmain.write('<td>%s</td>\n' % variable)
        fmain.write('</tr>')

        # Exchange loop
        for locale in self.looper.exchanges.locales():
            fmain.write('<tr><td>%s</td>' % locale)

            for variable in self.looper.variables.variables():
                fmain.write('<td>')

                for ndays in Ndays.ndays():
                    name = get_name_locale( locale, variable, ndays )
                    fmain.write('<a href="html_%s/hstr_%s.html">%dd</a> ' % (variable, name, ndays))

                fmain.write('</td>')

            fmain.write('</tr>\n')

        # End of table
        fmain.write('</table>\n')

        fmain.write('</html>\n')
        fmain.close()

        return

    def html_other( self ):
        for variable in self.looper.variables.variables():
            views = Views(variable)
            for ndays in Ndays.ndays():
                for locale in self.looper.exchanges.locales():
                    self.html_locale(variable, ndays, locale)
        return

    def html_finish( self ):
        # Move the files
        for variable in self.looper.variables.variables():
            dirname = 'html_%s' % variable
            cmdC1 = 'xcopy /S /Y /I %s \\\\smrc-ltc-mrct50\\data00\\tickMon\\webmon\\%s' % (dirname, dirname)
            cmdC2 = 'xcopy /S /Y hstrTickSumm.html \\\\smrc-ltc-mrct50\\data00\\tickMon\\webmon\\'
            cmdD1 = 'rmdir /S /Q %s' % dirname
            cmdD2 = 'del hstrTickSumm.html'
            os.system(cmdC1)
            os.system(cmdC2)
            os.system(cmdD1)
            os.system(cmdD2)
        return

    def html_locale( self, variable, ndays, locale ):
        name = get_name_locale( locale, variable, ndays )
        flocale = open('html_%s/hstr_%s.html' % (variable, name), 'w')
        flocale.write('<html>\n')

        lines = tabMenu('hstrTickSumm', 1)
        for l in lines:
            flocale.write(l)

        flocale.write('<table>\n')

        # links to other markets
        flocale.write('<table>\n')
        flocale.write('<tr><td>')
        for locale2 in self.looper.exchanges.locales():
            if locale2 != locale:
                name = get_name_locale( locale2, variable, ndays )
                flocale.write('<a href="hstr_%s.html">%s</a> ' % (name, locale2) )
            else:
                flocale.write('%s ' % locale2 )
        flocale.write('</td></tr>')

        # links to other date ranges
        flocale.write('<tr><td>')
        for ndays2 in Ndays.ndays():
            if ndays2 != ndays:
                name = get_name_locale( locale, variable, ndays2 )
                flocale.write('<a href="hstr_%s.html">%dd</a> ' % (name, ndays2))
            else:
                flocale.write('%dd ' % ndays2)
        flocale.write('</td></tr>')

        views = Views(variable)
        for view in views.views():
            title = get_title(variable, locale, view)
            flocale.write('<tr><td><img src="../png_%s/%s.%d.png"></td></tr>\n' % (variable, title, ndays))

        flocale.write('</table>\n')
        flocale.write('</html>\n')
        flocale.close()
        return

    def get_time( self, date, ndays = 0 ):
        dt = datetime.datetime( date.year, date.month, date.day) + datetime.timedelta(days=ndays)
        seconds = (dt - self.t0).days*24*60*60 + (dt - self.t0).seconds
        return seconds
        return

    def get_datelist( self, date1, date2 ):
        datelist = []
        for i in range( (date2 - date1).days + 1 ):
            datelist.append( date1 + datetime.timedelta(days = i) )
        return datelist;

    def idate2date( self, idate ):
        date = datetime.date( idate/10000, idate%10000/100, idate%100)
        return date

class AlertLooper:
    def __init__(self):
        self.variables = Variables()
        self.exchanges = Exchanges()
        self.t0 = datetime.datetime(1970, 1, 1, 0);
        self.now = datetime.datetime.utcnow()
        return

    def valovis(self):
        valovis = []
        for variable in self.variables.variables():
            views = Views(variable)
            for locale in self.exchanges.markets():
                for view in views.viewsSimple():
                    valovi = [variable, locale, view]
                    valovis.append(valovi)
        return valovis

    def currValues(self):
        currValues = []
        for variable in self.variables.variables():
            views = Views(variable)
            for locale in self.exchanges.markets():
                for view in views.viewsSimple():
                    currDate = self.get_currDate( variable, locale, view )
                    val = 0.0
                    if 'filesize' == variable:
                        dest_level = get_dest_level(variable, locale, view)
                        val = get_filesize( locale, view, dest_level, currDate )
                    currValues.append(val)
        return currValues

class AlertMaker:
    def __init__( self ):
        self.variables = Variables()
        self.exchanges = Exchanges()
        self.t0 = datetime.datetime(1970, 1, 1, 0);
        self.now = datetime.datetime.utcnow()
        return

    def run( self ):
        valovis = []
        for variable in self.variables.variables():
            views = Views(variable)
            for locale in self.exchanges.markets():
                for view in views.viewsSimple():
                    valovi = [variable, locale, view]
                    valovis.append(valovi)

        currValues = []
        for variable in self.variables.variables():
            views = Views(variable)
            for locale in self.exchanges.markets():
                for view in views.viewsSimple():
                    currDate = self.get_currDate( variable, locale, view )
                    val = 0.0
                    if 'filesize' == variable:
                        dest_level = get_dest_level(variable, locale, view)
                        val = get_filesize( locale, view, dest_level, currDate )
                    currValues.append(val)

        allValues = []
        # date loop
        for variable in self.variables.variables():
            views = Views(variable)
            for locale in self.exchanges.markets():
                for view in views.viewsSimple():
                    values = []
                    for date in self.get_datelist():
                        val = 0.0
                        if 'filesize' == variable:
                            dest_level = get_dest_level(variable, locale, view)
                            val = get_filesize( locale, view, dest_level, date )
                        if val > 1e-6: # don't use zeros
                            values.append(val)
                    allValues.append(values)

        mean_stdev = [ self.get_mean_stdev_trim(series) for series in allValues ]

        falert = open('tickDataAlerts.html', 'w')
        falert.write('<html>\n')

        falert.write('<table>\n')
        falert.write('<tr><td colspan=4 align=right>Updated: %s, UTC</td></tr>\n' % self.now.ctime())
        falert.write('<tr><td colspan=4>[')
        falert.write('<a href=liveTickSumm.html>Live Tick Summary</a> | ')
        falert.write('<a href=hstrTickSumm.html>Tick Data History</a> | ')
        falert.write('<a href=usBookSumm.html>US Book Data</a> | ')
        falert.write('Tick Data Alerts')
        falert.write(']</td></tr>')

        falert.write('<tr><td colspan=4><hr></td></tr>\n')

        for i, cv in enumerate(currValues):
            m = mean_stdev[i][0]
            s = mean_stdev[i][1]
            if math.fabs(cv - m) > 3.0 * s:
                header = '%s: %s of %s' % ( Exchanges.city(valovis[i][1]), Variables.fullname(valovis[i][0]), Views.fullname(valovis[i][2]) )
                name = get_name_locale(valovis[i][1], valovis[i][0], 50)
                lnk = '<a href=html_%s/hstr_%s.html>%s</a>' % (valovis[i][0], name, header)
                sig = 0.0
                if s > 0:
                    sig = (cv-m)/s
                rat = 0.0
                if m > 0:
                    rat = cv/m*100.0
                stat1 = '(%.1f sig from 30day avg %d +- %d, ' % ( sig, int(m), int(s) )
                stat2 = '%.0f %% of the avg.)' % rat
                falert.write('<tr><td>%s</td><td>= %d</td><td>%s</td><td>%s</td></tr>' % (lnk, cv, stat1, stat2))

        falert.write('</table>\n')
        falert.write('</html>')
        falert.close()

        cmdC1 = 'xcopy /S /Y tickDataAlerts.html \\\\smrc-ltc-mrct50\\data00\\tickMon\\webmon\\'
        cmdD1 = 'del tickDataAlerts.html'
        os.system(cmdC1)
        os.system(cmdD1)

        return

    def get_mean_stdev_trim( self, series ):
        if len(series) < 1:
            return [0, 0]

        trimmed = self.trim_series(series)
        if len(trimmed) < 1:
            return [0, 0]

        ret = self.get_mean_stdev( trimmed )
        return ret

    def get_mean_stdev( self, series ):
        if len(series) < 2:
            return [0, 0]

        mean = float(sum(series)) / len(series)
        sdsq = sum([(i - mean) ** 2 for i in series])
        stdev = (sdsq / (len(series) - 1)) ** .5

        return [mean, stdev]

    def trim_series( self, series ):
        trimmed = []
        length = len(series)
        series.sort()
        lowerb = series[int(length*0.1)]
        upperb = series[int(length*0.9)]
        for s in series:
            if s >= lowerb and s <= upperb:
                trimmed.append(s)
        return trimmed

    def get_datelist( self ):
        datenow = self.now.date()
        datelist = []
        for i in range(40):
            if len(datelist) >= 20:
                break
            newdate = datenow - datetime.timedelta(days = i)
            if newdate.weekday() < 5:
                datelist.append( newdate )
        return datelist;

    def get_currDate( self, variable, locale, view ):
        currDate = self.now.date()

        if 'filesize' == variable:

            renewTime = datetime.time(0, 0)
            if locale in self.exchanges.markets('EURO'):
                renewTime = datetime.time(18, 0)
            elif locale in self.exchanges.markets('CAN'):
                renewTime = datetime.time(23, 30)
            elif locale in self.exchanges.markets('ASIA'):
                renewTime = datetime.time(9, 0)

            if renewTime > self.now.time():
                currDate = currDate - datetime.timedelta(days = 1)
                while currDate.weekday() >= 5: # Sat. and Sun.
                    currDate = currDate - datetime.timedelta(days = 1)

        return currDate
