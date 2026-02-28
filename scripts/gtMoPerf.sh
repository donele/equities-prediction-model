#!/bin/bash
baseDir="/home/jelee/hffit"
mList=CA

NT=0
if (( $# >= 1 )); then
	MKT=$1
	if (( $# == 2 )); then
		NT=$2
	fi
fi

function setNtrees {
	model=$1
	mm=$2
	nOm=30
	nTm=30
	if (( NT > 0 )); then
		nTrees=$NT
	elif [[ "x$model" == "xUS" ]]; then
		if (( mm > 201711 )); then
			nOm=60
		fi
		nTm=100
		#if (( mm > 201705 )); then
			#nTm=125
		#fi
	elif [[ "x$model" == "xUF" ]]; then
		nOm=60
		nTm=45
		if (( mm > 201705 )); then
			nTm=65
		fi
	elif [[ "x$model" == "xCA" ]]; then
		nTm=45
		if (( mm > 201705 )); then
			nTm=65
		fi
	elif [[ "x$model" == "xEU" ]]; then
		nTm=65
		if (( mm > 201705 )); then
			nTm=125
		fi
	elif [[ "x$model" == "xAT" ]]; then
		nTm=40
		if (( mm > 201705 )); then
			nTm=100
		fi
	elif [[ "x$model" == "xAH" ]]; then
		nTm=40
	elif [[ "x$model" == "xAS" ]]; then
		nTm=45
	elif [[ "x$model" == "xKR" ]]; then
		nTm=100
	elif [[ "x$model" == "xSS" ]]; then
		nTm=25
	elif [[ "x$model" == "xAA" ]]; then
		nTm=30
	fi
	let "nTrees = nOm * 1000 + nTm"
}

function printStat {
	model=$1

	echo $model
	for mm in 201701 201702 201703 201704 201705 201706 201707 201708 201709 201710 201711 201712 201801 201802 201803 201804 201805 201806 201807 201808 201809 201810; do
		echo $mm
		setNtrees $model $mm
		#tar="tar60;0_xbmpred60;0_tar600;60_1.0_tar3600;660_0.5"
		#if [[ ("x${model:0:1}" == "xU" && $mm > 201607) || ($mm > 201904) ]]; then
			tar="tar60;0_tar600;60_1.0_tar3600;660_0.5"
		#fi
		dir=${baseDir}/${model}/fit/${tar}/stat_0

		echo -n $mm; grep $nTrees ${dir}/anaNTreeMultTar_${mm}*${mm}*.txt;
	done;
}

if [[ "x$MKT" != "x" ]]; then
	printStat $MKT
else
	for model in $mList; do
		printStat $model
	done
fi


