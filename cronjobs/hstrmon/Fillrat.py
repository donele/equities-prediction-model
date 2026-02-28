import os
import math
import string
import sys
import datetime
import time
import pyodbc
from ROOT import gPad, gStyle, gROOT, TCanvas, TH1F, TLegend, TLine, TLatex, TColor
from WebMon import *

doCopy = True
plotWidth = 750
projectName = 'fillrat'
mondir = '\\\\smrc-ltc-mrct50\\data00\\tickMon\\webmon'

dictDB = {}
dictDB['EURO'] = pyodbc.connect('DSN=hfstockeu;UID=mercator1;PWD=DBacc101')
dictDB['ASIA'] = pyodbc.connect('DSN=hfstockasia;UID=mercator1;PWD=DBacc101')
dictDB['CAN'] = pyodbc.connect('DSN=hfstocktsx;UID=mercator1;PWD=DBacc101')
dictDB['MA'] = pyodbc.connect('DSN=hfstockma;UID=mercator1;PWD=DBacc101')

dateList = {}
dateList['EURO'] = {20090626: 'CoLo', 20090902: 'ECN', 20091023: 'Unfake', 20091109: 'Gap Detect', 20091123: 'Latency Check', 20100210: 'CHXD', 20100212: 'exclude ETF', 20100303: 'LSE Direct'}
dateList['ASIA'] = {20091023: 'Unfake', 20091026: 'Tokyo Experiment with cancel', 20091116: 'Tokyo Expr ends', 20091123: 'Latency Check'}
dateList['CAN'] = {20090609: 'ECN', 20090831: 'Alpha', 20091023: 'Unfake', 20091120: 'Latency Check'}
dateList['MA'] = {}

#------------------------------------------------------------------------------
# Variables, Exchanges, Views, and Ndays
#------------------------------------------------------------------------------

class ExecTypes:
    def __init__( self ):
        self.types_ = ['L', 'A']

    def fullname( type ):
        if 'L' == type:
            return 'MT'
        elif 'A' == type:
            return 'MM'
    fullname = staticmethod(fullname)

    def types( self ):
        return self.types_

class SchedTypes:
    def __init__( self ):
        self.types_ = [1, 2]

    def fullname( type ):
        if 1 == type:
            return 'synchronous'
        elif 2 == type:
            return 'dataDriven'
    fullname = staticmethod(fullname)

    def types( self ):
        return self.types_

class Variables:
    def __init__( self, execType ):
        if 'L' == execType:
            self.variables_ = ['fillratincl', 'fillratnonfake', 'ratdirectfill', 'ratmatched', 'ratfake', 'ratcomp']
        else:
            self.variables_ = ['fillratincl', 'fillratnonfake', 'ratdirectfill', 'ratmatched']

    def fullname( variable ):
        if 'fillratincl' == variable:
            return 'Fill Ratio'
        elif 'fillratnonfake' == variable:
            return 'Fill on Non-fake orders'
        elif 'ratdirectfill' == variable:
            return 'Fill prompt'
        elif 'ratmatched' == variable:
            return 'Order-Tickdata match'
        elif 'ratfake' == variable:
            return 'Order sent against fake'
        elif 'ratcomp' == variable:
            return 'Nonfill due to competitors'
        return variable
    fullname = staticmethod(fullname)

    def lineColor( variable ):
        if 'fillratincl' == variable:
            return 2
        elif 'fillratnonfake' == variable:
            return 12
        elif 'ratdirectfill' == variable:
            return 4
        elif 'ratmatched' == variable:
            return 3
        elif 'ratfake' == variable:
            return 46
        elif 'ratcomp' == variable:
            return 38
        return 5
    lineColor = staticmethod(lineColor)

    def markerStyle( variable ):
        if 'fillratincl' == variable:
            return 22
        elif 'fillratnonfake' == variable:
            return 23
        elif 'ratdirectfill' == variable:
            return 24
        elif 'ratmatched' == variable:
            return 26
        elif 'ratfake' == variable:
            return 5
        elif 'ratcomp' == variable:
            return 27
        return 5
    markerStyle = staticmethod(markerStyle)

    def variables( self ):
        return self.variables_

class Dests:
    def __init__( self ):
        return

    def fullname( dest ):
        if 'primary' == dest:
            return 'Primary'
        elif 'ET' == dest:
            return 'Turquoise'
        elif 'EH' == dest:
            return 'Chi-X'
        elif 'EG' == dest:
            return 'BATS'
        elif 'CC' == dest:
            return 'Chi-X'
        elif 'CH' == dest:
            return 'Alpha'
        elif 'CO' == dest:
            return 'Omega'
        elif 'CP' == dest:
            return 'Pure'
        return ''
    fullname = staticmethod(fullname)

    def dests( self, locale ):
        ex = Exchanges()
        if 'EURO' == ex.region(locale):
            self.dest = ['primary', 'ET', 'EH', 'EG']
        elif 'ASIA' == ex.region(locale):
            self.dest = ['primary']
        elif 'CAN' == ex.region(locale):
            self.dest = ['primary', 'CC', 'CH', 'CO', 'CP']
        elif 'MA' == ex.region(locale):
            self.dest = ['primary']
        return self.dest

class Exchanges:

    def __init__( self ):
        self.exch = {}
        self.exch['EURO'] = ['EA', 'EB', 'EI', 'EP', 'EF', 'EL', 'ED', 'EM', 'EZ', 'EO', 'EX', 'EC', 'EW', 'EY']
        self.exch['ASIA'] = ['AT', 'AH', 'AS', 'AK', 'AQ', 'AG', 'AW']
        self.exch['CAN'] = ['CJ']
        self.exch['MA'] = ['MJ']

        #self.exch['EURO'] = ['EA']
        #self.exch['ASIA'] = ['AT', 'AH', 'AS', 'AK', 'AQ', 'AG', 'AW']
        #self.exch['CAN'] = ['CJ']
        #self.exch['MA'] = ['MJ']

    def locales( self, region = '' ):
        ret = []
        if '' == region:
            ret += ['EURO']
            for r in self.exch:
                ret += self.exch[r]
        elif region in self.exch:
            if 'EURO' == region:
                ret += ['EURO']
            ret += self.exch[region]

        return ret

    def markets( self, locale = '' ):
        ret = []
        if '' == locale or 'ALL' == locale:
            for r in self.exch:
                ret += self.exch[r]
        elif self.is_region(locale):
            ret = self.exch[locale]
        else:
            ret.append(locale)
        return ret

    def is_region( self, locale ):
        return locale in self.regions()

    def regions( self ):
        ret = []
        ret += [ r for r in self.exch ]
        return ret

    def region( self, locale ):
        ret = ''
        if locale in self.regions():
            ret = locale
        else:
            for r in self.exch:
                if locale == r or locale in self.exch[r]:
                    ret = r
        return ret

class Ndays:
    def __init__( self ):
        return

    def ndays():
        return [20, 100, 250, 750, 1250]
        #return [10]
    ndays = staticmethod(ndays)

class DateList:
    def __init__( self ):
        return

    def list( region ):
        date2 = datetime.datetime.utcnow().date()
        date1 = date2 - datetime.timedelta( days = Ndays.ndays()[-1]*1.6 )
        idate1 = date1.year * 10000 + date1.month * 100 + date1.day
        idate2 = date2.year * 10000 + date2.month * 100 + date2.day

        dateStart = datetime.datetime.utcnow()
        #print dateStart
        sys.stdout.flush()

        datelistDB = []
        cursor = dictDB[region].cursor()
        cursor.execute('''select distinct idate from hfordersumm
                          where idate >= %d and idate <= %d
                          order by idate''' % (idate1, idate2))
        for row in cursor:
            datelistDB.append(row.idate)

        dateEnd = datetime.datetime.utcnow()
        #print 'query in DateList.list() took %d seconds' % (dateEnd - dateStart).seconds
        #sys.stdout.flush()

        datelist = []
        n = 0
        while(1):
            d = date1 + n * datetime.timedelta(1)
            wd = d.weekday()
            idate = d.year * 10000 + d.month * 100 + d.day
            if wd < 5:
                datelist.append(idate)
            if idate >= datelistDB[0]:
                break
            n += 1

        datelist += datelistDB

        return datelist
    list = staticmethod(list)

#------------------------------------------------------------------------------
# Loopers and Makers
#------------------------------------------------------------------------------

class PlotMaker:
    def __init__( self, idate = 0 ):
        self.dests = Dests()
        self.exchanges = Exchanges()
        self.execTypes = ExecTypes()
        self.schedTypes = SchedTypes()
        self.datelist = {}
        for r in self.exchanges.regions():
            self.datelist[r] = DateList.list(r)

        gROOT.SetStyle('Plain')
        gStyle.SetOptStat(0000)
        gStyle.SetTitleH(0.09)
        gStyle.SetTitleW(0.65)
        gStyle.SetTitleX(0.02)
        gStyle.SetLabelSize(0.04, "y")
        gStyle.SetLabelSize(0.03, "x")
        gStyle.SetPadTopMargin(0.13)
        gStyle.SetPadBottomMargin(0.13)

    def plot_fillrat( self ):

        for execType in self.execTypes.types():
            self.variables = Variables(execType)
            for schedType in self.schedTypes.types():
                for locale in self.exchanges.locales():
                    for dest in self.dests.dests(locale):

                        # each locale
                        mVal = {}
                        for variable in self.variables.variables():
                            values = self.get_fillrat( variable, dest, locale, execType, schedType )
                            mVal[variable] = values

                        # plot
                        self.make_plot_fillrat( dest, locale, execType, schedType, mVal )

        if doCopy:
            cmdC1 = 'xcopy /S /Y /I gif_%s %s\\gif_%s' % (projectName, mondir, projectName)
            cmdD1 = 'rmdir /S /Q gif_%s' % projectName
            os.system(cmdC1)
            os.system(cmdD1)

    def get_fillrat( self, variable, dest, locale, execType, schedType ):
        region = self.exchanges.region(locale)
        values = [0 for i in range(len(self.datelist[region]))]

        mDateVal = {}
        if locale not in self.exchanges.regions():
            if 'primary' == dest:
                dest = locale
            region = self.exchanges.region(locale)
            cursor = dictDB[region].cursor()

            dateStart = datetime.datetime.utcnow()
            #print dateStart
            sys.stdout.flush()

            cmd = '''select idate, %s from hfordersumm
                where dest = '%s' and exchange = '%s' and exectype = '%s' and schedType = %d
                and norder > 0
                and idate >= %d and idate <= %d''' % (variable, dest[1], locale[1], execType, schedType, self.datelist[region][0], self.datelist[region][-1])
            cursor.execute(cmd)

            dateEnd = datetime.datetime.utcnow()
            #print 'first query in get_fillrat() took %d seconds\n' % (dateEnd - dateStart).seconds
            #sys.stdout.flush()

            for row in cursor:
                idate = int(row[0])
                val = row[1]
                mDateVal[idate] = val

        elif 'EURO' == locale:
            if 'fillratincl' == variable:
                numerName = 'nfillincl'
                denomName = 'norder'
            elif 'fillratnonfake' == variable:
                numerName = 'nfillnonfake'
                denomName = 'nordernonfake'
            elif 'ratdirectfill' == variable:
                numerName = 'ndirectfill'
                denomName = 'nmatched'
            elif 'ratmatched' == variable:
                numerName = 'nmatched'
                denomName = 'norder'
            elif 'ratfake' == variable:
                numerName = 'norderfake'
                denomName = 'nmatched'
            elif 'ratcomp' == variable:
                numerName = 'ncomp'
                denomName = 'nmatched - norderfake'

            mDateVal = {}
            cursor = dictDB[region].cursor()
            selDest = ' dest = exchange '
            if 'primary' != dest:
                selDest = " dest = '%s' " % dest[1]

            dateStart = datetime.datetime.utcnow()
            #print dateStart
            sys.stdout.flush()

            cmd = '''select idate, sum(%s), sum(%s) from hfordersumm
                where %s and exectype = '%s' and schedType = %d
                and idate >= %d and idate <= %d
                group by idate''' % (numerName, denomName, selDest, execType, schedType, self.datelist[region][0], self.datelist[region][-1])
            cursor.execute(cmd)
            for row in cursor:
                idate = int(row[0])
                numer = float(row[1])
                denom = float(row[2])
                val = 0
                if denom > 0:
                    val = numer / denom
                mDateVal[idate] = val

            dateEnd = datetime.datetime.utcnow()
            #print 'second query in get_fillrat() took %d seconds\n' % (dateEnd - dateStart).seconds
            #sys.stdout.flush()

        for i in range(len(self.datelist[region])):
            idate = self.datelist[region][i]
            val = 0
            if idate in mDateVal:
                val = mDateVal[idate]
            values[i] = val

        return values


    def make_plot_fillrat( self, dest, locale, execType, schedType, mVal ):
        region = self.exchanges.region(locale)
        gifdir = 'gif_%s' % projectName
        if os.path.isdir(gifdir):
            pass
        else:
            os.mkdir(gifdir)

        for nd in Ndays.ndays():
            gifn = self.get_gifname( dest, locale, execType, schedType, nd )

            mVarHist = {}
            for variable, values in mVal.iteritems():
                name = 'hist_%s_%s' % (variable, gifn)
                title = self.get_title( variable, dest, locale, execType, schedType )
                hist = TH1F(name, title, nd, 0, nd)

                b = 1 # b = 1, 2, ..., nd
                for v in values[-nd:]:
                    hist.SetBinContent(b, v)
                    if nd / 40 == 0 or (nd - b) % (nd / 40) == 0:
                        sdate = `self.datelist[region][-(nd-b)-1]`
                        hist.GetXaxis().SetBinLabel(b, sdate)
                    b = b + 1

                hist.SetMarkerStyle(4)
                hist.SetMarkerSize(0.8)

                mVarHist[variable] = hist

            canv = TCanvas("c1", "c1", plotWidth, 450)
            leg = TLegend(0.7, 0.82, 0.99, 0.98)

            gPad.SetGrid()
            histCnt = 0
            for variable, hist in mVarHist.iteritems():
                hist.SetLineColor(Variables.lineColor(variable))
                hist.SetMarkerStyle(Variables.markerStyle(variable))
                if histCnt == 0:
                    hist.SetTitle(self.get_title('', dest, locale, execType, schedType))
                    hist.SetMinimum(0)
                    hist.SetMaximum(1.1)
                    hist.Draw('LP')
                else:
                    hist.Draw('LP,same')
                histCnt += 1

            for variable in self.variables.variables():
                leg.AddEntry(mVarHist[variable], Variables.fullname(variable), 'LP')
            leg.SetFillStyle(0)
            leg.Draw()

            mLines = {}
            for k, v in dateList[region].iteritems():
                if k in self.datelist[region][-nd:]:
                    x = self.datelist[region][-nd:].index(k)
                    line = TLine(x, 0, x, 1)
                    line.SetLineColor(2)
                    line.SetLineStyle(4)
                    line.SetLineWidth(2)
                    mLines[k] = line
            for k, v in mLines.iteritems():
                v.Draw()

            #tex = TLatex(1.15, 0.05, "Preliminary")
            #tex.SetTextColor(TColor.GetColor('#99cccc'))
            #tex.SetTextAngle(26)
            #tex.SetTextSize(0.1)
            #tex.Draw()

            canv.Print('gif_%s/%s' % (projectName, gifn))

    def get_title( self, variable, dest, locale, execType, schedType ):
        if 'EURO' == locale:
            locale = 'All EU'
        title = '%s primary= %s, dest= %s, type= %s %s' % (variable, locale, dest, ExecTypes.fullname(execType), SchedTypes.fullname(schedType))
        return title

    def get_gifname( self, dest, locale, execType, schedType, nd ):
        if 'primary' == dest:
            dest = locale
        gifn = '%s_%s_%s_%d_%d.gif' % (dest, locale, execType, schedType, nd)
        return gifn

    def html( self ):
        self.html_main()
        self.html_other()
        self.html_finish()

    def html_main( self ):
        htmldir = 'html_%s' % projectName
        if os.path.isdir(htmldir):
            pass
        else:
            os.mkdir(htmldir)

        fmain = open('%sSumm.html' % projectName, 'w')

        listLocale = self.exchanges.locales()
        listDest = self.dests.dests(self.exchanges.region(listLocale[0]))
        listNdays = Ndays.ndays()
        htmlname = self.get_htmlname(listDest[0], listLocale[0], 'L', 1, listNdays[-1])
        fmain.write('<head><meta http-equiv="Refresh" content="0; URL=%s/%s"></head>' % (htmldir, htmlname) )

    def get_htmlname( self, dest, locale, execType, schedType, ndays ):
        if 'primary' == dest:
            deste = locale
        htmlname = '%s_%s_%s_%d_%s.html' % (dest, locale, execType, schedType, ndays)
        return htmlname

    def html_other( self ):
        for locale in self.exchanges.locales():
            region = self.exchanges.region(locale)
            for dest in self.dests.dests(region):
                for execType in self.execTypes.types():
                    for schedType in self.schedTypes.types():
                        for ndays in Ndays.ndays():
                            self.html_locale(dest, locale, execType, schedType, ndays)

    def html_finish( self ):
        # Move the files
        if doCopy:
            cmdC1 = 'xcopy /S /Y /I html_%s %s\\html_%s' % (projectName, mondir, projectName)
            cmdC2 = 'xcopy /S /Y %sSumm.html %s\\' % (projectName, mondir)
            cmdD1 = 'rmdir /S /Q html_%s' % projectName
            cmdD2 = 'del %sSumm.html' % projectName
            os.system(cmdC1)
            os.system(cmdC2)
            os.system(cmdD1)
            os.system(cmdD2)

    def html_locale( self, dest, locale, execType, schedType, ndays):
        region = self.exchanges.region(locale)
        htmlname = self.get_htmlname(dest, locale, execType, schedType, ndays)
        flocale = open('html_%s/%s' % (projectName, htmlname), 'w')
        flocale.write('<html>\n')

        lines = tabMenu('%sSumm' % projectName, 1)
        for l in lines:
            flocale.write(l)

        # links to the regions
        flocale.write('Region [')
        for region2 in self.exchanges.regions():
            listDest = self.dests.dests(region2)
            listLocale = self.exchanges.locales(region2)
            listNdays = Ndays.ndays()
            if region2 != region:
                htmlname = self.get_htmlname(listDest[0], listLocale[0], execType, schedType, ndays)
                flocale.write('<a href="%s">%s</a> ' % (htmlname, region2) )
            else:
                flocale.write('<b>%s</b> ' % (region2) )
        flocale.write(']')

        flocale.write('<br>\n')

        # links to other destinations
        flocale.write('Destination [')
        for dest2 in self.dests.dests(region):
            if dest2 != dest:
                htmlname = self.get_htmlname(dest2, locale, execType, schedType, ndays)
                flocale.write('<a href="%s">%s</a> ' % (htmlname, Dests.fullname(dest2)) )
            else:
                flocale.write('<b>%s</b> ' % Dests.fullname(dest2) )
        flocale.write(']')

        flocale.write('<br>\n')

        # links to other markets
        flocale.write('Primary Exch [')
        for locale2 in self.exchanges.locales(region):
            if locale2 != locale:
                htmlname = self.get_htmlname(dest, locale2, execType, schedType, ndays)
                flocale.write('<a href="%s">%s</a> ' % (htmlname, locale2) )
            else:
                flocale.write('<b>%s</b> ' % locale2 )
        flocale.write(']')

        flocale.write('<br>\n')

        # links to other execTypes
        flocale.write('[')
        for execType2 in self.execTypes.types():
            if execType2 != execType:
                htmlname = self.get_htmlname(dest, locale, execType2, schedType, ndays)
                flocale.write('<a href="%s">%s</a> ' % (htmlname, ExecTypes.fullname(execType2)) )
            else:
                flocale.write('<b>%s</b> ' % ExecTypes.fullname(execType2) )
        flocale.write(']')

        flocale.write('\n')

        # links to other schedTypes
        flocale.write('<td>[')
        for schedType2 in self.schedTypes.types():
            if schedType2 != schedType:
                htmlname = self.get_htmlname(dest, locale, execType, schedType2, ndays)
                flocale.write('<a href="%s">%s</a> ' % (htmlname, SchedTypes.fullname(schedType2)) )
            else:
                flocale.write('<b>%s</b> ' % SchedTypes.fullname(schedType2) )
        flocale.write(']')

        flocale.write('\n')

        # links to other date ranges
        flocale.write('[')
        for ndays2 in Ndays.ndays():
            if ndays2 != ndays:
                htmlname = self.get_htmlname(dest, locale, execType, schedType, ndays2)
                flocale.write('<a href="%s">%dd</a> ' % (htmlname, ndays2))
            else:
                flocale.write('<b>%dd</b> ' % ndays2)
        flocale.write(']')

        flocale.write('\n')

        # plots
        flocale.write('<table>\n')
        gifn = self.get_gifname(dest, locale, execType, schedType, ndays)

        flocale.write('<tr>')

        # fill ratio
        flocale.write('<td width=%d><img src="../gif_%s/%s"></td>\n' % (plotWidth, projectName, gifn))

        # special dates
        flocale.write('<td valign=top>')
        flocale.write('Notable Dates:<br>\n')
        keys = dateList[region].keys()
        keys.sort()
        #for k, v in dateList[region].iteritems():
        for k in keys:
            flocale.write('<b>%d</b>: %s<br>\n' % (k, dateList[region][k]))
        flocale.write('</td>')

        flocale.write('</tr>\n')

        flocale.write('</table>\n')
        flocale.write('</html>\n')
        flocale.close()

    def html_get_legend( self ):
        lines = []
        lines.append('<table>\n')
        lines.append('<tr><td colspan=2><b>Fill Ratio</b> = (number of filled (partial or full) orders) / (number of total orders)</td></tr>\n')
        lines.append('<tr><td colspan=2><b>Fill on Non-fake orders</b> = (number of filled (partial or full) orders that are sent against non-fake orders) / (number of total orders that are send agains non-fake orders)</td></tr>\n')
        lines.append('<tr><td colspan=2><b>Fill Prompt</b> = (number of filled (partial or full) orders that received the fill report within a predefined time limit) / (number of matched orders)</td></tr>\n')
        lines.append('<tr><td colspan=2><b>Order-Tickdata match</b> = (number of orders that are successfully matched to the tickdata) / (number of total orders)</td></tr>\n')
        lines.append('<tr><td colspan=2><b>Order sent against fake</b> = (number of orders that are sent against nonfake orders) / (number of matched orders) (Market Taking only)</td></tr>\n')
        lines.append('<tr><td colspan=2><b>Nonfill due to competitors</b> = (number of orders that are not filled when there is a faster trade) / (number of matched orders) (Market Taking only)</td></tr>\n')
        lines.append('</table>\n')
        return lines
