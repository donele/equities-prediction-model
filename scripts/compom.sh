#!/bin/bash
cmodel=$1
idate=$2

model.sh index 20 $idate $idate && \
	model.sh sample om 60 $idate $idate && \
	mx comp_om.conf -a "set cmodel $cmodel" -a "set dateFrom $idate" -a "set dateTo $idate" >> comp_om.log

