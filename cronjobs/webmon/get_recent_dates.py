#!/usr/bin/python

import datetime
import sys

def get_idate( t ):
    return (int)(t.strftime("%Y%m%d"))

ndates = (int)(sys.argv[1])
argc = len(sys.argv)
oneday = datetime.timedelta(days = 1)

if argc == 2:
#   d = datetime.date.today()
   now = datetime.datetime.utcnow()
   d = now.date()
   t = now.time()
   #if t.hour > 9:
      #d = d + oneday
else:
   idate = (int)(sys.argv[2])
   d = datetime.datetime(idate/10000, idate/100%100, idate%100)

printed = 0
ofile = open( 'temp_recent_dates.txt', 'w' )
ofile.write( '%d\n' % ndates )
while printed < ndates:
      while d.weekday() > 4:
            d = d - oneday
      ofile.write( '%d\n' % get_idate(d) )
      printed = printed + 1
      d = d - oneday
