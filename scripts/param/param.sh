#!/bin/bash

# Function definitions.
function usage {
echo "Usage:"
echo "  ${0##*/} calculate US"
echo "  ${0##*/} calculate US 20150101"
echo "  ${0##*/} write US"
echo "  ${0##*/} write US 20150101"
}

function valid_model {
[[ "x$1" == "xUS" || "x$1" == "xUF" || "x$1" == "xCA" || "x$1" == "xEU" \
	|| "x$1" == "xAT" || "x$1" == "xAH" || "x$1" == "xKR" || "x$1" == "xAS" \
	|| "x$1" == "xMJ" || "x$1" == "xSS" || "x$1" == "xAA" ]]
}

function valid_action {
[[ "x$1" == "xcalculate" || "x$1" == "xwrite" ]]
}

function valid_date {
(( $1 > 20000000 && $1 < 30000000 ))
}

function get_log_file {
log_head=$1
log_base=${log_head}
log_ext='log'
cnt=2
while [[ -e $log_dir/${log_base}.${log_ext} ]]; do
	log_base="${log_head}_${cnt}"
	(( ++cnt ))
done
log_file="$log_dir/${log_base}.${log_ext}"
}
# END Function definitions.


# Set the overnight job flag.
OVERNIGHT_JOB=false
if [[ "x$HOSTNAME" == *"smrc-chi22"* ]]; then
	OVERNIGHT_JOB=true
	source ~/.bashrc
fi

# Parse args and set action, model, and udate.
until (( $# < 1 )) || valid_action $1; do
	if [[ "$1" == "-p" ]]; then
		print=-p
	fi
	shift
done
if (( $# < 2 )); then
	usage
	exit 1
else
	action=$1
	model=$2
	if (( $# == 2 )); then
		if [[ "x$action" == "xcalculate" ]]; then
			udate=`get_idate -m US`
		elif [[ "x$action" == "xwrite" ]]; then
			udate=`get_idate -m US -dd 1 -lw -b`
		fi
		if ! valid_date $udate; then
			echo Failed call to get_idate. >&2
			exit 1
		fi
	else
		udate=$3
	fi
fi
if ! valid_action $action || ! valid_model $model || ! valid_date $udate; then
	echo "Invalid parameter: $*" >&2
	usage
	exit 1
fi

# Set the log directory
yyyymmdd=$udate
yyyy=$(( $udate / 10000 ))
yyyymm=$(( $udate / 100 ))
log_dir="$HOME/hffit2/log/param/$yyyy/$yyyymm"
if [[ "x$OVERNIGHT_JOB" == "xtrue" ]]; then
	log_dir="$HOME/paramjobs/hffit/log/param/$yyyy/$yyyymm"
fi
if [[ ! -e $log_dir ]]; then
	mkdir -p $log_dir || {
	echo "Cannot create log directory $log_dir" >&2
	exit 1
}
fi

# Set the log file name.
case $action in
	calculate)
		log_base=param_${model}_${udate}
		get_log_file $log_base
		;;
	write)
		log_base=write_${model}_${udate}
		get_log_file $log_base
		;;
	*)
		usage
		;;
esac
date > $log_file

# Working directory
param_dir=$HOME/scripts/param
if [[ "x$OVERNIGHT_JOB" == "xtrue" ]]; then
	param_dir="$HOME/paramjobs/scripts/param"
fi
cd $param_dir || {
echo "Cannot change to $param_dir" >> $log_file
exit 1
}

# Output directory
if [[ ! -e hffit ]]; then
	if [[ "$OVERNIGHT_JOB" == "true" ]]; then
		ln -s $HOME/paramjobs/hffit hffit
	else
		ln -s $HOME/hffit2 hffit
	fi
else
	if [[ ! -d hffit ]]; then
		echo "ERROR: hffit needs to be a directory." >> $log_file
		exit 1
	fi
fi

case $action in
	calculate)
		mx param_index.conf -a "set model $model" -a "set udate $udate" $print >> $log_file 2>&1 && mx param_sample.conf -a "set model $model" -a "set udate $udate" $print >> $log_file 2>&1
		;;
	write)
		if [[ "x$model" == "xUS" ]]; then
			mx write_index.conf -a "set model $model" -a "set udate $udate" $print >> $log_file 2>&1 && mx write_weight_US.conf -a "set model $model" -a "set udate $udate" $print >> $log_file 2>&1
		elif [[ "x$model" == "xUF" ]]; then
			mx write_weight.conf -a "set model $model" -a "set udate $udate" $print >> $log_file 2>&1
		else
			mx write_index.conf -a "set model $model" -a "set udate $udate" $print >> $log_file 2>&1 && mx write_weight.conf -a "set model $model" -a "set udate $udate" $print >> $log_file 2>&1
		fi
		;;
	*)
		usage
		;;
esac
echo Return Value: $? >> $log_file
date >> $log_file
