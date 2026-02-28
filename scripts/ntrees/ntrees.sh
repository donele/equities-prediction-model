#!/bin/bash
# US ntrees experient.

# Function definitions.
function usage {
echo "Usage:"
echo "  ${0##*/} US"
}

function valid_model {
[[ "x$1" == "xUS" || "x$1" == "xUF" || "x$1" == "xCA" || "x$1" == "xEU" \
	|| "x$1" == "xAT" || "x$1" == "xAH" || "x$1" == "xKR" || "x$1" == "xAS" \
	|| "x$1" == "xMJ" || "x$1" == "xSS" || "x$1" == "xAA" ]]
}

function do_event {
[[ "x$1" == "xUS" || "x$1" == "xUF" || "x$1" == "xCA" || "x$1" == "xEU" ]]
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

# Parse args and set model, and udate.
dateArg=false
until (( $# < 1 )) || valid_model $1; do
	if [[ "$1" == "-p" ]]; then
		print=-p
	fi
	shift
done
if (( $# < 1 )); then
	usage
	exit 1
else
	model=$1
	if (( $# == 1 )); then
		#udate=`get_idate -m US`
		udate=`get_idate -m US -dd 1 -lw -b`
		if ! valid_date $udate; then
			echo Failed call to get_idate. >&2
			exit 1
		fi
	else
		udate=$2
		dateArg=true
	fi
fi
if ! valid_model $model || ! valid_date $udate; then
	echo "Invalid parameter: $*" >&2
	usage
	exit 1
fi

# Set the log directory
yyyymmdd=$udate
yyyy=$(( $udate / 10000 ))
yyyymm=$(( $udate / 100 ))
log_dir="$HOME/hffit2/log/ntrees/$yyyy/$yyyymm"
if [[ "x$OVERNIGHT_JOB" == "xtrue" ]]; then
	log_dir="$HOME/paramjobs/hffit/log/ntrees/$yyyy/$yyyymm"
fi
if [[ ! -e $log_dir ]]; then
	mkdir -p $log_dir || {
	echo "Cannot create log directory $log_dir" >&2
	exit 1
}
fi

# Set the log file name.
log_base=ntrees_${model}_${udate}
get_log_file $log_base
date > $log_file

# Working directory
param_dir=$HOME/scripts/ntrees
if [[ "x$OVERNIGHT_JOB" == "xtrue" ]]; then
	param_dir="$HOME/paramjobs/scripts/ntrees"
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

do_om=true
do_tm=true
do_ome=true

if [[ "$do_om" == "true" ]]; then
	mx sample_om_auto.conf -a "set model $model" $print >> $log_file 2>&1 && \
	mx pred_om_auto.conf -a "set model $model" $print >> $log_file 2>&1 && \
	if [[ $dateArg == "true" ]]; then
		mx ntrees_om_test.conf -a "set model $model" -a "set dateFrom $udate" -a "set dateTo $udate" $print >> $log_file 2>&1
	else
		mx ntrees_om.conf -a "set model $model" $print >> $log_file 2>&1
	fi
fi

if [[ "$do_tm" == "true" ]]; then
	mx sample_tm_auto.conf -a "set model $model" $print >> $log_file 2>&1 && \
	mx pred_tm_auto.conf -a "set model $model" $print >> $log_file 2>&1 && \
	if [[ $dateArg == "true" ]]; then
		mx ntrees_tm_test.conf -a "set model $model" -a "set dateFrom $udate" -a "set dateTo $udate" $print >> $log_file 2>&1
	else
		mx ntrees_tm.conf -a "set model $model" $print >> $log_file 2>&1
	fi
fi

if [[ "$do_ome" == "true" ]]; then
if do_event $model; then
	mx sample_ome_auto.conf -a "set model $model" $print >> $log_file 2>&1 && \
	mx pred_ome_auto.conf -a "set model $model" $print >> $log_file 2>&1 && \
	if [[ $dateArg == "true" ]]; then
		mx ntrees_ome_test.conf -a "set model $model" -a "set dateFrom $udate" -a "set dateTo $udate" $print >> $log_file 2>&1
	else
		mx ntrees_ome.conf -a "set model $model" $print >> $log_file 2>&1
	fi
fi
fi

echo Return Value: $? >> $log_file
date >> $log_file
