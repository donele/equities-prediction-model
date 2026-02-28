#!/usr/bin/python

import string

fo = open('disk_alert.txt', 'w')
sizes = {}
sizes['\\\\SMRC-NJ-MRCT15\\\F'] = 1799855534080.0/1024/1024
sizes['\\\\SMRC-NJ-MRCT14\\\F'] = 1799855534080.0/1024/1024
sizes['\\\\SMRC-CHI-MRCT10\\\F'] = 1799839285248.0/1024/1024
limit = {}
limit['\\\\SMRC-NJ-MRCT15\\\F'] = 30*1024;
limit['\\\\SMRC-NJ-MRCT14\\\F'] = 60*1024;
limit['\\\\SMRC-CHI-MRCT10\\\F'] = 10*1024;

f = open('diruse.txt', 'r')
lines = f.readlines()
for l in lines:
    sl = string.split(l)
    if len(sl) == 4 and sl[2] == 'TOTAL:':
       dir = sl[3]
       used = float(sl[0])
       total = sizes[dir]
       free = total - used
       if free < limit[dir]:
          fo.write('%s is only %d GB free\n' % (dir, free/1024)
