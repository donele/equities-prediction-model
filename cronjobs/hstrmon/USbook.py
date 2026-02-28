import os
import math
import string
import sys
import datetime
import time
import pyodbc
from ROOT import gPad, gStyle, gROOT, TCanvas, TH1F
from WebMon import *

#------------------------------------------------------------------------------
# Variables, Exchanges, Views, and Ndays
#------------------------------------------------------------------------------

class Variables:
    def __init__( self ):
        self.variables_ = ['rawSize', 'rawClose', 'obSizeChi', 'nObChi', 'tobSize', 'bookOk', 'nbboQSize', 'nbboTSize', 'nbboRSize']
        return

    def abbrev( variable ):
        if 'rawSize' == variable:
            return 'rs'
        elif 'rawClose' == variable:
            return 'rc'
        elif 'obSizeChi' == variable:
            return 'osc'
        elif 'nObChi' == variable:
            return 'noc'
        elif 'tobSize' == variable:
            return 'ts'
        elif 'nbboQSize' == variable:
            return 'nqs'
        elif 'nbboTSize' == variable:
            return 'nts'
        elif 'nbboRSize' == variable:
            return 'nrs'
        elif 'bookOk' == variable:
            return 'ok'
        return variable
    abbrev = staticmethod(abbrev)

    def fullname( variable ):
        if 'rawSize' == variable:
            return 'Raw data size (MB)'
        elif 'rawClose' == variable:
            return 'Raw data close (Hours after close)'
        elif 'obSizeChi' == variable:
            return 'Orderbook size Chicago (MB)'
        elif 'nObChi' == variable:
            return 'N Orderbook files Chicago'
        elif 'tobSize' == variable:
            return 'TOB size (MB)'
        elif 'nbboQSize' == variable:
            return 'NBBO Quote size (MB)'
        elif 'nbboTSize' == variable:
            return 'NBBO Trade size (MB)'
        elif 'nbboRSize' == variable:
            return 'NBBO Return size (MB)'
        elif 'bookOk' == variable:
            return 'Orderbook Data OK'
        return variable
    fullname = staticmethod(fullname)

    def variables( self ):
        return self.variables_

class Exchanges:

    def __init__( self ):
        self.mkt = ['UP', 'UQ', 'UN', 'UC', 'UD', 'UJ']
        return

    def fullname(locale):
        if 'ALL' == locale:
            return 'All the books'
        elif 'UP' == locale:
            return 'ARCA'
        elif 'UQ' == locale:
            return 'INET'
        elif 'UN' == locale:
            return 'NYSE'
        elif 'UC' == locale:
            return 'BATS'
        elif 'UD' == locale:
            return 'EDGX'
        elif 'UJ' == locale:
            return 'EDGA'
        return locale
    fullname = staticmethod(fullname)

    def locales( self ):
        ret = ['ALL']
        for r in self.mkt:
            ret.append(r)
        return ret

    def markets( self, region = '' ):
        ret = []
        if 'ALL' == region:
            ret = self.mkt
        elif '' == region:
            ret = self.mkt
        return ret

    def regions( self ):
        ret = ['ALL']
        return ret

    def is_region( self, locale ):
        return 'ALL' == locale

class Ndays:
    def __init__ ( self ):
        return

    def ndays():
        return [20, 100, 250, 500, 1000]
        #return [20]
    ndays = staticmethod(ndays)

class DateList:
    def __init__( self ):
        return

    def list():
        calDaysMax = Ndays.ndays()[-1] * 2
        date2 = datetime.datetime.utcnow().date()
        date1 = date2 - datetime.timedelta( days = calDaysMax )

        datelist = []
        for i in range( (date2 - date1).days + 1 ):
            newdate = date1 + datetime.timedelta(days = i)
            if newdate.weekday() < 5: # Mon - Fri
                datelist.append( newdate )

        return datelist
    list = staticmethod(list)

#------------------------------------------------------------------------------
# Loopers and Makers
#------------------------------------------------------------------------------

class PlotMaker:
    def __init__( self, idate = 0 ):
        self.variables = Variables()
        self.exchanges = Exchanges()

        self.datelist = DateList.list()
        self.dataok = self.get_dataok()
        self.nbbo_variables = ['nbboQSize', 'nbboTSize', 'nbboRSize']

        gROOT.SetStyle('Plain')
        gStyle.SetOptStat(0000)
        gStyle.SetTitleH(0.12)
        gStyle.SetTitleX(0.2)
        gStyle.SetLabelSize(0.1, "y")
        gStyle.SetLabelSize(0.12, "x")
        gStyle.SetPadTopMargin(0.17)
        gStyle.SetPadBottomMargin(0.15)

        return

    def date2sdate( self, date ):
        sdate = '%d' % (date.year * 10000 + date.month * 100 + date.day)
        return sdate

    def date2idate( self, date ):
        idate = date.year * 10000 + date.month * 100 + date.day
        return idate

    def get_dataok( self ):
        db = pyodbc.connect('DSN=equitydata;UID=mercator1;PWD=DBacc101')
        cursor = db.cursor()
        sdate = self.date2sdate(self.datelist[0])
        cursor.execute('''select idate, arcaOK, inetOK, nyseOK, batsOK, edgxOK, edgaOK
                          from tickdataelvok
                          where idate >= %s''' %  sdate)

        self.mmDataok = { 'UP': {}, 'UQ': {}, 'UN': {}, 'UC': {}, 'UD': {}, 'UJ': {} }
        for row in cursor:
            self.mmDataok['UP'][row.idate] = self.bool2int(row.arcaOK)
            self.mmDataok['UQ'][row.idate] = self.bool2int(row.inetOK)
            self.mmDataok['UN'][row.idate] = self.bool2int(row.nyseOK)
            self.mmDataok['UC'][row.idate] = self.bool2int(row.batsOK)
            self.mmDataok['UD'][row.idate] = self.bool2int(row.edgxOK)
            self.mmDataok['UJ'][row.idate] = self.bool2int(row.edgaOK)
        return

    def bool2int( self, b ):
        i = 0
        if b == True:
            i = 1
        else:
            i = 0
        return i

    def plot( self ):
        variables_not_summed = ['rawClose', 'nbboQSize', 'nbboTSize', 'nbboRSize']

        for variable in self.variables.variables():

            # each market
            mVal = {}
            for market in self.exchanges.markets():
                values = self.get_values( variable, market )
                mVal[market] = values

            # sum (or max) over the markets
            for region in self.exchanges.regions():
                values = []
                for i in range(len(self.datelist)):
                    val = 0.0
                    #if 'rawClose' == variable: # get maximum
                    if variable in variables_not_summed:
                        for locale in self.exchanges.markets(region):
                            if val < mVal[locale][i]:
                                val = mVal[locale][i]
                    else: # get the sum
                        for locale in self.exchanges.markets(region):
                            val = val + mVal[locale][i]
                    values.append(val)
                mVal[region] = values

            # plot
            for locale in self.exchanges.locales():
                if variable not in self.nbbo_variables or 'ALL' == locale:
                    self.make_plot( variable, locale, mVal[locale] )

        cmdC1 = 'xcopy /S /Y /I gif_usbook \\\\smrc-ltc-mrct50\\data00\\tickMon\\webmon\\gif_usbook'
        cmdD1 = 'rmdir /S /Q gif_usbook'
        os.system(cmdC1)
        os.system(cmdD1)

        return

    def get_values( self, variable, locale ):
        values = []

        if 'rawSize' == variable:
            for d in self.datelist:
                values.append( self.get_rawSize( locale, d ) )
        elif 'rawClose' == variable:
            for d in self.datelist:
                values.append( self.get_rawClose( locale, d ) )
        elif 'obSizeChi' == variable:
            for d in self.datelist:
                values.append( self.get_obSizeChi( locale, d ) )
        elif 'nObChi' == variable:
            for d in self.datelist:
                values.append( self.get_nObChi( locale, d ) )
        elif 'tobSize' == variable:
            for d in self.datelist:
                values.append( self.get_tobSize( locale, d ) )
        elif 'nbboQSize' == variable:
            for d in self.datelist:
                values.append( self.get_nbboQSize( d ) )
        elif 'nbboTSize' == variable:
            for d in self.datelist:
                values.append( self.get_nbboTSize( d ) )
        elif 'nbboRSize' == variable:
            for d in self.datelist:
                values.append( self.get_nbboRSize( d ) )
        elif 'bookOk' == variable:
            for d in self.datelist:
                idate = self.date2idate(d)
                value = 0
                if idate in self.mmDataok[locale]:
                    value = self.mmDataok[locale][idate]
                values.append( value )

        return values

    def get_tobSize( self, market, date ):
        path = self.get_tobPath( market, date )
        size = self.get_filesize( path )
        return size

    def get_nbboQSize( self, date ):
        paths = self.get_nbboQPaths( date )
        sum = 0.0
        for p in paths:
            sum += self.get_filesize( p )
        return sum

    def get_nbboTSize( self, date ):
        path = self.get_nbboTPath( date )
        size = self.get_filesize( path )
        return size

    def get_nbboRSize( self, date ):
        path = self.get_nbboRPath( date )
        size = self.get_filesize( path )
        return size

    def get_rawSize( self, market, date ):
        path = self.get_rawPath( market, date )
        size = self.get_filesize( path )
        return size

    def get_rawClose( self, market, date ):
        path = self.get_rawPath( market, date )
        hourClose = self.get_fileclose( path, date )
        return hourClose

    def get_obSizeChi( self, market, date ):
        pathbase = '\\\\smrc-ltc-nas03\\gf0\\book_us\\'
        obSize = self.get_obSize( pathbase, market, date )
        return obSize

    def get_obSize( self, pathbase, market, date ):
        paths = self.get_obPaths( pathbase, market, date )
        obSize = 0.0
        for p in paths:
            obSize += self.get_filesize(p)
        return obSize

    def get_nObChi( self, market, date ):
        pathbase = '\\\\smrc-ltc-nas03\\gf0\\book_us\\'
        nOb = self.get_nObFiles( pathbase, market, date )
        return nOb

    def get_nObFiles( self, pathbase, market, date ):
        paths = self.get_obPaths( pathbase, market, date )
        n = 0
        for p in paths:
            if os.path.exists(p):
                n = n + 1
        return n

    def get_tobPath( self, market, date ):
        pathbase = '\\\\smrc-ltc-nas03\\gf1\\tickC\\us\\book\\tob\\'
        path = '%s%s\\%d\\%d\\q%d_%d.bin' % (pathbase, market[1], date.year, date.year * 100 + date.month, date.year * 100 + date.month, date.day)
        return path

    def get_nbboQPaths( self, date ):
        paths = []
        pathbase = '\\\\smrc-ltc-nas03\\gf1\\tickC\\us\\book\\nbbo\\'
        for subdir in ['1', '2']:
            path = '%s%s\\%4d\\%6d\\q%6d_%d.bin' % (pathbase, subdir, date.year, date.year * 100 + date.month, date.year * 100 + date.month, date.day)
            paths.append(path)
        return paths

    def get_nbboTPath( self, date ):
        pathbase = '\\\\smrc-ltc-nas03\\gf1\\tickC\\us\\book\\nbbo'
        path = '%s\\1\\%4d\\%6d\\t%6d_%d.bin' % (pathbase, date.year, date.year * 100 + date.month, date.year * 100 + date.month, date.day)
        return path

    def get_nbboRPath( self, date ):
        pathbase = '\\\\smrc-ltc-nas03\\gf1\\tickC\\us\\book\\nbbo'
        path = '%s\\1\\%4d\\%6d\\re%8s.bin' % (pathbase, date.year, date.year * 100 + date.month, self.date2sdate(date))
        return path

    def get_rawPath( self, market, date ):
        pathbase = '\\\\smrc-ltc-nas03\\gf0\\mdcap\\'

        sdate = self.get_sdate( date )
        idate = self.date2idate( date )
        path = ''

        if 'UP' == market:
            path = pathbase + 'arca\\' + sdate + '_arca.7z'
            if idate < 20130206:
                path = pathbase + 'arcabook\\' + sdate + '_arca_pkt2.log.bz2'
                if idate < 20120523:
                    path = pathbase + 'arcabook\\' + sdate + '_arca_pkt.log.bz2'
                if idate < 20120127:
                    if idate < 20100920:
                        path = pathbase + 'arcabook\\' + sdate + '_arcam.log.bz2'
                    else:
                        path = pathbase + 'arcabook\\' + sdate + '_arcam_pkt.log.bz2'

        elif 'UQ' == market:
            path = pathbase + 'itch41\\' + sdate + '_itch41.7z'
            if idate < 20130206:
                path = pathbase + 'inetbook\\' + sdate + '_itch41_pkt2.log.bz2'
                if idate < 20120523:
                    if idate < 20090225:
                        path = pathbase + 'inetbook\\' + sdate + '_itch.log.bz2'
                    elif idate >= 20090225 and idate < 20100915:
                        path = pathbase + 'inetbook\\' + sdate + '_itch4.log.bz2'
                    elif idate >= 20100915 and idate < 20101012:
                        path = pathbase + 'inetbook\\' + sdate + '_itch4_pkt.log.bz2'
                    else:
                        path = pathbase + 'inetbook\\' + sdate + '_itch41_pkt.log.bz2'

        elif 'UN' == market:
            path = pathbase + 'nyse\\' + sdate + '_nyse.7z'
            if idate < 20130206:
                path = pathbase + 'nysebook\\' + sdate + '_nyseultra_pkt2.log.bz2'
                if idate < 20120523:
                    if idate < 20100920:
                        path = pathbase + 'nysebook\\' + sdate + '_nyseultra.log.bz2'
                    else:
                        path = pathbase + 'nysebook\\' + sdate + '_nyseultra_pkt.log.bz2'

        elif 'UC' == market:
            path = pathbase + 'bzx\\' + sdate + '_bzx.7z'
            if idate < 20130206:
                path = pathbase + 'batsbook\\' + sdate + '_bats_pkt2.log.bz2'
                if idate < 20120523:
                    if (idate < 20090226) or (idate > 20090304 and idate < 20091109):
                        path = pathbase + 'batsbook\\' + sdate + '_batsbook.log.bz2'
                    elif (idate >= 20090226 and idate <= 20090304) or (idate >= 20091109 and idate < 20100920):
                        path = pathbase + 'batsbook\\' + sdate + '_bats.log.bz2'
                    else:
                        path = pathbase + 'batsbook\\' + sdate + '_bats_pkt.log.bz2'

        elif 'UD' == market:
            path = pathbase + 'edgx\\' + sdate + '_edgx.7z'
            if idate < 20130206:
                path = pathbase + 'edgx\\' + sdate + '_edgxng_pkt2.log.bz2'
                if idate < 20120523:
                    if idate < 20100715:
                        path = pathbase + 'edgx\\' + sdate + '_edgx.log.bz2'
                    elif idate >= 20100715 and idate < 20100915:
                        path = pathbase + 'edgx\\' + sdate + '_edgxng.log.bz2'
                    else:
                        path = pathbase + 'edgx\\' + sdate + '_edgxng_pkt.log.bz2'

        elif 'UJ' == market:
            path = pathbase + 'edga\\' + sdate + '_edga.7z'
            if idate < 20130206:
                path = pathbase + 'edga\\' + sdate + '_edgang_pkt2.log.bz2'
                if idate < 20120523:
                    path = pathbase + 'edga\\' + sdate + '_edgang_pkt.log.bz2'
        return path

    def get_obPaths( self, pathbase, market, date ):
        paths = []
        subdir = ''
        if 'UP' == market:
            subdir = 'arcabook'
        elif 'UQ' == market:
            subdir = 'inetbook'
        elif 'UN' == market:
            subdir = 'nyseultrabook'
        elif 'UC' == market:
            subdir = 'batsbook'
        elif 'UD' == market:
            subdir = 'edgx'
        elif 'UJ' == market:
            subdir = 'edga'
        n = 20
        ticksize = 0.0
        for i in range(n):
            path = '%s%s\\%d\\%4d\\%6d\\ob%8s.bin' % (pathbase, subdir, i + 1, date.year, date.year * 100 + date.month, self.date2sdate(date))
            paths.append(path)
        return paths

    def get_filesize( self, path ):
        size = 0.0
        if os.path.exists(path):
            size = os.path.getsize(path)
        mb = size / 1024.0 / 1024.0

        return mb

    def get_fileclose( self, path, date ):
        hourClose = 0.0
        if os.path.exists(path):
            secEpoch = os.path.getmtime(path)
            cTime = time.ctime( secEpoch )
            if int(cTime[8:10]) == date.day:
                hh = int(cTime[11:13])
                if hh >= 15 and hh < 23:
                    hourClose = hh - 15 + float(cTime[14:16]) / 60.0 + float(cTime[17:19]) / 60.0 / 60.0

        #if self.is_dst(date):
            #hourClose = hourClose + 1.0
        return hourClose

    def is_dst(self, date):
        yyyy = date.year
        dst_start = self.get_dst_start(yyyy)
        dst_end = self.get_dst_end(yyyy)
        if date > dst_start and date < dst_end:
            return True
        return False

    def get_dst_start(self, yyyy): # second sunday of march
        date = datetime.date(yyyy, 3, 1)
        nSunday = 0
        for i in range(14):
            if date.weekday() == 6:
                nSunday = nSunday + 1
            if nSunday >= 2:
                break
            date = date + datetime.timedelta(days = 1)
        return date

    def get_dst_end(self, yyyy): # first sunday in november
        date = datetime.date(yyyy, 11, 1)
        nSunday = 0
        for i in range(7):
            if date.weekday() == 6:
                nSunday = nSunday + 1
            if nSunday >= 1:
                break
            date = date + datetime.timedelta(days = 1)
        return date

    def make_plot( self, variable, locale, values ):
        gifdir = 'gif_usbook'
        if os.path.isdir(gifdir):
            pass
        else:
            os.mkdir(gifdir)

        for nd in Ndays.ndays():
            gifn = self.get_gifname( variable, locale, nd )
            name = 'hist_%s' % gifn
            title = self.get_title( variable, locale )
            hist = TH1F(name, title, nd, 0, nd)

            b = 1 # b = 1, 2, ..., nd
            for v in values[-nd:]:
                hist.SetBinContent(b, v)
                if (nd - b) % (nd / 10) == 0:
                    sdate = self.get_sdate( self.datelist[-(nd-b)-1] )
                    hist.GetXaxis().SetBinLabel(b, sdate)
                b = b + 1

            hist.SetMarkerStyle(4)
            hist.SetMarkerSize(0.8)

            canv = TCanvas("c1", "c1", 480, 160)
            gPad.SetGrid()
            hist.Draw('LP')
            canv.Print('gif_usbook/%s' % gifn)

        return

    def get_sdate( self, date ):
        sdate = '%04d%02d%02d' % (date.year, date.month, date.day)
        return sdate

    def get_title( self, variable, locale ):
        if variable in self.nbbo_variables:
            title = Variables.fullname(variable)
        else:
            title = Exchanges.fullname(locale) + ' ' + Variables.fullname(variable)
        return title

    def get_gifname( self, variable, locale, nd ):
        if variable in self.nbbo_variables:
            gifn = '%s_%d.gif' % (Variables.abbrev(variable), nd)
        else:
            gifn = '%s_%s_%d.gif' % (locale, Variables.abbrev(variable), nd)
        return gifn

    def html( self ):
        self.html_main()
        self.html_other()
        self.html_finish()
        return

    def html_main( self ):
        htmldir = 'html_usbook'
        if os.path.isdir(htmldir):
            pass
        else:
            os.mkdir(htmldir)

        fmain = open('usBookSumm.html', 'w')
        fmain.write('<html>\n')

        lines = tabMenu('usBookSumm', 0)
        for l in lines:
            fmain.write(l)

        fmain.write('<table>\n')

        # Exchange loop
        for locale in self.exchanges.locales():
            fmain.write('<tr><td>%s</td>' % locale)

            fmain.write('<td>')

            for ndays in Ndays.ndays():
                fmain.write('<a href="html_usbook/%s_%s.html">%dd</a> ' % (locale, ndays, ndays))

            fmain.write('</td>')

            fmain.write('</tr>\n')

        # End of table
        fmain.write('</table>\n')

        fmain.write('</html>\n')
        fmain.close()

        return

    def html_other( self ):
        for locale in self.exchanges.locales():
            for ndays in Ndays.ndays():
                self.html_locale(locale, ndays)
        return

    def html_finish( self ):
        # Move the files
        cmdC1 = 'xcopy /S /Y /I html_usbook \\\\smrc-ltc-mrct50\\data00\\tickMon\\webmon\\html_usbook'
        cmdC2 = 'xcopy /S /Y usBookSumm.html \\\\smrc-ltc-mrct50\\data00\\tickMon\\webmon\\'
        cmdD1 = 'rmdir /S /Q html_usbook'
        cmdD2 = 'del usBookSumm.html'
        os.system(cmdC1)
        os.system(cmdC2)
        os.system(cmdD1)
        os.system(cmdD2)
        return

    def html_locale( self, locale, ndays):
        flocale = open('html_usbook/%s_%s.html' % (locale, ndays), 'w')
        flocale.write('<html>\n')

        lines = tabMenu('usBookSumm', 1)
        for l in lines:
            flocale.write(l)

        flocale.write('<table>\n')

        # links to other markets
        flocale.write('<table>\n')
        flocale.write('<tr><td>')
        for locale2 in self.exchanges.locales():
            if locale2 != locale:
                flocale.write('<a href="%s_%s.html">%s</a> ' % (locale2, ndays, locale2) )
            else:
                flocale.write('<b>%s</b> ' % locale2 )
        flocale.write('</td></tr>')

        # links to other date ranges
        flocale.write('<tr><td>')
        for ndays2 in Ndays.ndays():
            if ndays2 != ndays:
                flocale.write('<a href="%s_%s.html">%dd</a> ' % (locale, ndays2, ndays2))
            else:
                flocale.write('<b>%dd</b> ' % ndays2)
        flocale.write('</td></tr>')

        # plots
        n = len(self.variables.variables())
        #for variable in self.variables.variables():
        for i in range(n):
            variable = self.variables.variables()[i]
            gifn = self.get_gifname(variable, locale, ndays)
            if i % 2 == 0:
                flocale.write('<tr>\n')
            flocale.write('<td><img src="../gif_usbook/%s"></td>\n' % gifn)
            if i % 2 == 1:
                flocale.write('</tr>\n')

        flocale.write('</table>\n')
        flocale.write('</html>\n')
        flocale.close()
        return

    def get_time( self, date, ndays = 0 ):
        dt = datetime.datetime( date.year, date.month, date.day) + datetime.timedelta(days=ndays)
        seconds = (dt - self.t0).days*24*60*60 + (dt - self.t0).seconds
        return seconds
        return

    def idate2date( self, idate ):
        date = datetime.date( idate/10000, idate%10000/100, idate%100)
        return date
