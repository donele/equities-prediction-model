#!/bin/bash
# pmodel.sh
# Signal calculation, fitting, analysis for the production model.

# Funciton definitions.
function usage_udate {
echo "  ${0##*/} <market> <udate>"
echo "  ${0##*/} <market> <udate> auto"
}

function usage_index {
echo "  ${0##*/} <market> index"
echo "  ${0##*/} <market> index <udate>"
echo "  ${0##*/} <market> index <dateFrom> <dateTo>"
}

function usage_sample {
echo "  ${0##*/} <market> sample"
echo "  ${0##*/} <market> sample <udate>"
echo "  ${0##*/} <market> sample <dateFrom> <dateTo>"
echo "  ${0##*/} <market> sample <type>"
echo "  ${0##*/} <market> sample <type> <udate>"
echo "  ${0##*/} <market> sample <type> <dateFrom> <dateTo>"
}

function usage_fit {
echo "  ${0##*/} <market> fit <udate>"
echo "  ${0##*/} <market> fit <type> <udate>"
}

function usage_pred {
echo "  ${0##*/} <market> pred <udate> <dateFrom> <dateTo>"
echo "  ${0##*/} <market> pred <type> <udate> <dateFrom> <dateTo>"
}

function usage_ana {
echo "  ${0##*/} <market> ana <udate> <dateFrom> <dateTo>"
echo "  ${0##*/} <market> ana <udate1> <udate2> <dateFrom> <dateTo>"
echo "  ${0##*/} <market> ana <type> <udate> <dateFrom> <dateTo>"
echo "  ${0##*/} <market> ana <type> <udate1> <udate2> <dateFrom> <dateTo>"
echo "  ${0##*/} <market> ana <type> <dateFrom> <dateTo>"
}

function usage_write {
echo "  ${0##*/} <market> write <udate>"
echo "  ${0##*/} <market> write <type> <udate>"
echo "  ${0##*/} <market> write <type> <udate> <nTrees>"
}

function usage_end {
echo " market = US, CA, etc."
echo " type = om, tm, ome."
}

function usage {
echo "Usage:"
usage_udate
usage_index
usage_sample
usage_fit
usage_pred
usage_ana
usage_write
usage_end
}

function valid_market {
[[ "x$1" == "xUS" || "x$1" == "xUF" || "x$1" == "xCA" || "x$1" == "xEU" \
	|| "x$1" == "xAT" || "x$1" == "xAH" || "x$1" == "xKR" || "x$1" == "xAS" \
	|| "x$1" == "xMJ" || "x$1" == "xSS" || "x$1" == "xAA" ]]
}

function do_event {
[[ "x$1" == "xUS" || "x$1" == "xUF" || "x$1" == "xCA" || "x$1" == "xEU" || "x$1" == "xAT" ]]
}

function do_to_close {
[[ "x$1" == "xCA" || "x$1" == "xAT" || "x$1" == "xAH" || "x$1" == "xAS" || "x$1" == "xAA" || "x$1" == "xKR" ]]
}

function valid_action {
[[ "x$1" == "xindex" || "x$1" == "xsample" || "x$1" == "xfit" || "x$1" == "xpred" || "x$1" == "xana" || "x$1" == "xwrite" ]]
}

function valid_type {
[[ "x$1" == "xom" || "x$1" == "xtm" || "x$1" == "xtc" || "x$1" == "xome" || "x$1" == "xomtm" || "x$1" == "xtmtc" ]]
}

function valid_date {
[[ "x$1" == "x0" ]] || (( ($1 > 20000000 && $1 < 30000000) ))
}

function valid_range {
(( $1 > 20000000 && $1 < 30000000 && $2 > 20000000 && $2 < 30000000 && $1 <= $2 ))
}

function call_index {
if (( $# == 1 )); then
	# <market>
	mx index_auto.conf -a "set model $1" $print > $log_dir/index_$1_auto_$yyyymmdd.log 2>&1
elif (( $# == 2 )) && valid_date $2; then
	# <market> <udate>
	mx index_all.conf -a "set model $1" -a "set udate $2" $print > $log_dir/index_$1_u$2.log 2>&1
elif (( $# == 3 )) && valid_range $2 $3; then
	# <market> <ndatesFront> <dateFrom> <dateTo>
	mx index_range.conf -a "set model $1" -a "set dateFrom $2" -a "set dateTo $3" $print > $log_dir/index_$1_$2_$3.log 2>&1
fi
}

function call_sample {
if ! do_event $1 && [[ "x$2" == "xome" ]]; then
	echo "warning: " $1 "event job attempted."
fi

if (( $# == 2 )); then
	# <market> <om|tm|ome>
	mx sample_$2_auto.conf -a "set model $1" $print > $log_dir/sample_$1_$2_auto_$yyyymmdd.log 2>&1
elif (( $# == 3 )) && valid_date $3; then
	# <market> <om|tm|ome> <udate>
	mx sample_$2_all.conf -a "set model $1" -a "set udate $3" $print > $log_dir/sample_$1_$2_all_u$3.log 2>&1
elif (( $# == 4 )) && valid_range $3 $4; then
	# <market> <om|tm|ome> <dateFrom> <dateTo>
	mx sample_$2_range.conf -a "set model $1" -a "set dateFrom $3" -a "set dateTo $4" $print > $log_dir/sample_$1_$2_$3_$4.log 2>&1
fi
}

function call_sample_all {
if (( $# == 1 )); then
	call_sample $1 tm; call_sample $1 om
	if do_event $1; then
		call_sample $1 ome
	fi
elif (( $# == 2 )); then
	call_sample $1 tm $2; call_sample $1 om $2
	if do_event $1; then
		call_sample $1 ome $2
	fi
elif (( $# == 3 )); then
	call_sample $1 tm $2 $3; call_sample $1 om $2 $3
	if do_event $1; then
		call_sample $1 ome $2 $3
	fi
fi
}

function call_pred {
if (( $# == 5 )) && valid_date $3 && valid_range $4 $5; then
	# <market> <om|tm|tc|ome> <udate> <dateFrom> <dateTo>
	mx pred_$2.conf -a "set model $1" -a "set udate $3" -a "set dateFrom $4" -a "set dateTo $5" $print > $log_dir/pred_$1_$2_u$3_$4_$5.log 2>&1
fi
}

function call_pred_all {
	call_pred $1 om $2 $3 $4; call_pred $1 tm $2 $3 $4
	if do_event $1; then
		call_pred $1 ome $2 $3 $4
	fi
	if do_to_close $1; then
		call_pred $1 tc $2 $3 $4
		call_pred $1 tmtc $2 $3 $4
	fi
}

function call_fit {
if (( $# == 3 )) && valid_type $2 && valid_date $3; then
	# <market> <om|tm|tc|ome> <udate>
	mx fit_$2.conf -a "set model $1" -a "set udate $3" $print > $log_dir/fit_$1_$2_u$3.log 2>&1
fi
}

function call_fit_all {
if (( $# == 2 )) && valid_date $2; then
	call_fit $1 om $2
	call_fit $1 tm $2
	if do_event $1; then
		call_fit $1 ome $2
	fi
	if do_to_close $1; then
		call_fit $1 tc $2
	fi
fi
}

function call_ana {
if (( $# == 5 )) && valid_date $3 && valid_range $4 $5; then
	# <market> <om|tm|ome> <udate> <dateFrom> <dateTo>
	mx ana_$2.conf -a "set model $1" -a "set udate $3" -a "set dateFrom $4" -a "set dateTo $5" $print > $log_dir/ana_$1_$2_u$3_$4_$5.log 2>&1
elif (( $# == 4 )) && valid_range $3 $4; then
	# <market> <om|tm|ome> <dateFrom> <dateTo>
	mx ana_$2.conf -a "set model $1" -a "set udate 0" -a "set dateFrom $3" -a "set dateTo $4" $print > $log_dir/ana_$1_$2_$3_$4.log 2>&1
fi
}

function call_ana_all {
	call_ana $1 om $2 $3 $4; call_ana $1 tm $2 $3 $4; call_ana $1 omtm $2 $3 $4
	if do_event $1; then
		call_ana $1 ome $2 $3 $4
	fi
	if do_to_close $1; then
		call_ana $1 tc $2 $3 $4
		call_ana $1 tmtc $2 $3 $4
		call_ana $1 omtmtc $2 $3 $4
	fi
}

function call_write {
type=$2
if do_to_close $1 && [[ "x${type}" == "xtm" ]]; then
	type=tmtc
fi

if (( $# == 3 )) && valid_date $3; then
	mx write_${type}.conf -a "set model $1" -a "set udate $3" $print > $log_dir/write_$1_${type}_$3.log 2>&1
elif (( $# == 4 )) && valid_date $3; then
	mx write_${type}.conf -a "set model $1" -a "set udate $3" -a "set nTrees $4" $print > $log_dir/write_$1_${type}_$3.log 2>&1
fi
}

function call_write_all {
if (( $# == 2 )); then
	call_write $1 om $2; call_write $1 tm $2
	if do_event $1; then
		call_write $1 ome $2
	fi
fi
}
# END Function definitions.


# Set the overnight job flag.
OVERNIGHT_JOB=false
if [[ "x$HOSTNAME" == *"smrc-chi22"* ]]; then
  OVERNIGHT_JOB=true
fi

# Working directory.
model_dir=$HOME/scripts/model
if [[ "x$OVERNIGHT_JOB" == "xtrue" ]]; then
  model_dir=$HOME/paramjobs/scripts/model
fi
cd $model_dir || {
  echo "Cannot change to $model_dir" >&2
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
    echo ERROR: hffit needs to be a directory.
    exit 1
  fi
fi

# Log directory.
yyyymmdd=`date +"%Y%m%d"`
yyyy=`date +"%Y"`
yyyymm=`date +"%Y%m"`
log_dir=$HOME/hffit2/log/model/$yyyy/$yyyymm
if [[ "x$OVERNIGHT_JOB" == "xtrue" ]]; then
  log_dir=$HOME/paramjobs/hffit/log/model/$yyyy/$yyyymm
fi
if [[ ! -e $log_dir ]]; then
	mkdir -p $log_dir || {
	echo "Cannot create log directory $log_dir" >&2
	exit 1
}
fi

# Parse args.
until (( $# < 1 )) || valid_market $1; do
	if [[ "x$1" == "x-p" ]]; then
		print=-p
	else
		echo "ERROR parsing."
		exit 1
	fi
	shift
done
# $1 = model (US, CA, etc.)
# $2 = index|sample|fit|ana
if [[ "x$OVERNIGHT_JOB" == "xtrue" ]]; then
  echo "ERROR: pmodel.sh not allowed on this machine."
  exit 1

  if (( $# == 0 )); then
    echo "ERROR: No args."
    usage
    exit 1
  elif [[ "x$2" != "xindex" ]]; then
    echo "ERROR: [$2] not allowed on this machine $HOSTNAME"
    exit 1
  fi
fi
if (( $# == 1 )); then
	echo "Invalid args" $*
	usage
	exit 1
elif (( $# == 2 )) && valid_market $1 && valid_date $2; then
	(call_index $1 $2; call_sample $1 om $2) &
	call_sample $1 tm $2 &
	if do_event $1; then
		call_sample $1 ome $2 &
	fi
	wait

	call_fit $1 tm $2
	call_fit $1 om $2
	if do_event $1; then
		call_fit $1 ome $2
	fi
	if do_to_close $1; then
		call_fit $1 tc $2
	fi
elif (( $# == 3 )) && valid_market $1 && valid_date $2 && [[ "$3" == "auto" ]]; then
	(call_index $1; call_sample $1 om) &
	call_sample $1 tm &
	if do_event $1; then
		call_sample $1 ome &
	fi
	wait

	call_fit $1 tm $2
	call_fit $1 om $2
	if do_event $1; then
		call_fit $1 ome $2
	fi
	if do_to_close $1; then
		call_fit $1 tc $2
	fi
elif valid_market $1 && valid_action $2; then
	case "$2" in
		index)  # index -------------------------------------------------------------
			if (( $# == 2 )); then
				call_index $1
			elif (( $# == 3 )); then
				call_index $1 $3
			elif (( $# == 4 )); then
				call_index $1 $3 $4
			else
				echo "Invalid args" $*
				echo "Usage:"
				usage_index
				exit 1
			fi
			;;
		sample) # sample ------------------------------------------------------------
			if (( $# >= 3 )) && valid_type $3; then
				if (( $# == 3 )); then
					call_sample $1 $3
				elif (( $# == 4 )); then
					call_sample $1 $3 $4
				elif (( $# == 5 )); then
					call_sample $1 $3 $4 $5
				fi
			else
				if (( $# == 2 )); then
					call_sample_all $1
				elif (( $# == 3 )) && valid_date $3; then
					call_sample_all $1 $3
				elif (( $# == 4 )) && valid_range $3 $4; then
					call_sample_all $1 $3 $4
				else
					echo "Invalid args" $*
					echo "Usage:"
					usage_sample
					exit 1
				fi
			fi
			;;
		fit)    # fit ---------------------------------------------------------------
			if (( $# == 4 )) && valid_type $3 && valid_date $4; then
				call_fit $1 $3 $4
			elif (( $# == 3 )) && valid_date $3; then
				call_fit_all $1 $3
			else
				echo "Invalid args" $*
				echo "Usage:"
				usage_fit
				exit 1
			fi
			;;
		pred)    # pred -------------------------------------------------------------
			if (( $# == 6 )) && valid_type $3; then
				# <type> <udate> <dateFrom> <dateTo>
				call_pred $1 $3 $4 $5 $6
			elif (( $# == 7 )) && valid_type $3; then
				# <type> <udate1> <udate2> <dateFrom> <dateTo>
				call_pred $1 $3 $4 $6 $7
				call_pred $1 $3 $5 $6 $7
			elif (( $# == 5 )) && valid_date $3 && valid_range $4 $5; then
				# <udate> <dateFrom> <dateTo>
				call_pred_all $1 $3 $4 $5
			elif (( $# == 6 )) && valid_date $3 && valid_date $4 && valid_range $5 $6; then
				# <udate1> <udate2> <dateFrom> <dateTo>
				call_pred_all $1 $3 $5 $6
				call_pred_all $1 $4 $5 $6
			else
				echo "Invalid args" $*
				echo "Usage:"
				usage_pred
				exit 1
			fi
			;;
		ana)    # ana ---------------------------------------------------------------
			if (( $# == 6 )) && valid_type $3; then
				# <type> <udate> <dateFrom> <dateTo>
				call_ana $1 $3 $4 $5 $6
			elif (( $# == 7 )) && valid_type $3; then
				# <type> <udate1> <udate2> <dateFrom> <dateTo>
				call_ana $1 $3 $4 $6 $7
				call_ana $1 $3 $5 $6 $7
			elif (( $# == 5 )) && valid_type $3; then
				# <type> <dateFrom> <dateTo>
				call_ana $1 $3 $4 $5
			elif (( $# == 5 )) && valid_date $3 && valid_range $4 $5; then
				# <udate> <dateFrom> <dateTo>
				call_ana_all $1 $3 $4 $5
			elif (( $# == 6 )) && valid_date $3 && valid_date $4 && valid_range $5 $6; then
				# <udate1> <udate2> <dateFrom> <dateTo>
				call_ana_all $1 $3 $5 $6
				call_ana_all $1 $4 $5 $6
			else
				echo "Invalid args" $*
				echo "Usage:"
				usage_ana
				exit 1
			fi
			;;
		write)  # write -------------------------------------------------------------
			if (( $# == 4 )) && valid_type $3 && valid_date $4; then
				call_write $1 $3 $4
			elif (( $# == 5 )) && valid_type $3 && valid_date $4 && (( $5 > 0 )); then
				call_write $1 $3 $4 $5
			elif (( $# == 3 )) && valid_date $3; then
				call_write_all $1 $3
			fi
			;;
		*)
		echo "Invalid args" $*
		usage
		exit 1
	esac
else
	echo "Invalid args" $*
	usage
	exit 1
fi

exit_status=$?
if (( $exit_status != 0 )); then
	echo $*
fi
exit $exit_status
