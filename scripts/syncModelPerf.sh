#!/bin/bash

for m in US UF CA EU AT AH AS KR AA MJ SS; do
	tarom="tar60;0"
	#tarom="tar60;0_xbmpred60;0"
	#if [[ "x${m:0:1}" == "xU" ]]; then
		#tarom="tar60;0"
	#fi
	rsync -a ~/hffit2/$m/fit/${tarom}_tar600\;60_1.0_tar3600\;660_0.5/stat_0/anaNT*.txt /mnt/l/tickMon/modelPerf/$m
done
