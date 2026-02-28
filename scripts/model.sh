#!/bin/bash

function valid_date {
(( $1 == 0 || ($1 > 20000000 && $1 < 30000000) ))
}

function valid_range {
(( $1 > 20000000 && $1 < 30000000 && $2 > 20000000 && $2 < 30000000 && $1 <= $2 ))
}

function valid_type {
[[ "x$1" == "xom" || "x$1" == "xtm" || "x$1" == "xome" || "x$1" == "xomtm" ]]
}

# $1 = index|sample|fit|ana
case "$1" in
# -----------------------------------------------------------------------------
# index
# -----------------------------------------------------------------------------
	index)
		if (( $# == 4 )) && valid_range $3 $4; then
			mx index.conf -a "set ndatesFront $2" -a "set dateFrom $3" -a "set dateTo $4" > index_$2_$3.log 2>&1
		else
			echo "Usage: $0 index <ndatesFront> <dateFrom> <dateTo>"
			exit 1
		fi
		;;
# -----------------------------------------------------------------------------
# sample
# -----------------------------------------------------------------------------
	sample)
		if (( $# == 5 )) && valid_range $4 $5; then
			if valid_type $2; then
				mx sample_$2.conf -a "set ndatesFront $3" -a "set dateFrom $4" -a "set dateTo $5" > sample_$2_$4_$5.log 2>&1
			fi
		else
			echo "Usage: $0 sample <type> <ndatesFront> <dateFrom> <dateTo>"
		fi
		;;
# -----------------------------------------------------------------------------
# fit
# -----------------------------------------------------------------------------
	fit)
		if (( $# == 3 )) && valid_date $3; then
			if valid_type $2; then
				mx fit_$2.conf -a "set udate $3" > fit_$2_u$3.log 2>&1
			fi
		else
			echo "Usage: $0 fit <type> <udate>"
		fi
		;;
# -----------------------------------------------------------------------------
# pred
# -----------------------------------------------------------------------------
	pred)
		if (( $# == 5 )) && valid_date $3 && valid_range $4 $5; then
			if valid_type $2; then
				mx pred_$2.conf -a "set udate $3" -a "set dateFrom $4" -a "set dateTo $5" > pred_$2_u$3.log 2>&1
			fi
		else
			echo "Usage: $0 pred <type> <udate> <dateFrom> <dateTo>"
		fi
		;;
# -----------------------------------------------------------------------------
# ana
# -----------------------------------------------------------------------------
	ana)
		if (( $# == 5 )) && valid_date $3 && valid_range $4 $5; then
			if valid_type $2; then
				mx ana_$2.conf -a "set udate $3" -a "set dateFrom $4" -a "set dateTo $5" > ana_$2_u$3.log 2>&1
			fi
		elif (( $# == 4 )) && valid_range $3 $4; then
			if valid_type $2; then
				mx ana_$2.conf -a "set udate 0" -a "set dateFrom $3" -a "set dateTo $4" > ana_$2_$3_$4.log 2>&1
			fi
		else
			echo "Usage: $0 ana <type> <udate> <dateFrom> <dateTo>"
		fi
		;;
# -----------------------------------------------------------------------------
# Usage
# -----------------------------------------------------------------------------
	*)
		echo
		echo "Usage:"
		echo "  ${0##*/} index <ndatesFront> <dateFrom> <dateTo>"
		echo "  ${0##*/} sample <type> <ndatesFront> <dateFrom> <dateTo>"
		echo "  ${0##*/} fit <type> <udate>"
		echo "  ${0##*/} pred <type> <udate> <dateFrom> <dateTo>"
		echo "  ${0##*/} ana <type> <udate> <dateFrom> <dateTo>"
		echo "  ${0##*/} ana <type> <dateFrom> <dateTo>"
		echo " type = om, tm, ome, omtm"
		exit 1
esac

