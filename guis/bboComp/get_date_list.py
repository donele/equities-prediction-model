#!/usr/bin/python

import os
import sys
import string

market = sys.argv[1]
inpath = sys.argv[2]
n = len(market) + 1

list_files = []
files = os.listdir(inpath)
nfiles = 0
for afile in files:
    filesize = os.stat( inpath + afile ).st_size
    if filesize > 6000:
       if afile[0:n] == market + '_' and afile[-5:] == '.root':
          nfiles = nfiles + 1
          list_files.append(afile)
dates = []
for afile in list_files:
    dates.append(int(afile[n:n+8]))

# Write the file with the information:
#   number of days available
#   list of dates
ofile = open( 'temp_dates.txt', 'w' )
ofile.write( '%d\n' % nfiles )
for date in sorted(dates):
    ofile.write( '%d\n' % date )

