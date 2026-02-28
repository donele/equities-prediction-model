#!/bin/bash
cmodel=$1
idate=$2

model.sh sample tm 0 $idate $idate && \
	mx comp_tm.conf -a "set cmodel $cmodel" -a "set dateFrom $idate" -a "set dateTo $idate" >> comp_tm.log

