#!/usr/bin/python

import os
import sys
import string

list_files = []
files = os.listdir('./data')
for afile in files:
    if afile[-4:] == '.txt':
       list_files.append(afile)

# Write the file with the information:
#   number of days available
#   list of dates
ofile = open( 'temp_files.txt', 'w' )
ofile.write( '%d\n' % len(list_files) )
for afile in list_files:
    ofile.write( '%s\n' % afile )

