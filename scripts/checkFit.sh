#!/bin/bash
function do_event {
[[ "x$1" == "xUS" || "x$1" == "xUF" || "x$1" == "xCA" || "x$1" == "xEU" ]]
}

# Working directory.
dir=$HOME/hffit2
cd $dir || {
echo "Cannot change to $dir" >&2
exit 1
}

for m in `ls -d ??`; do
	#tarom="tar60;0_xbmpred60;0"
	tarom="tar60;0"
	tartm="tar600;60_1.0_tar3600;660_0.5"
	#if [[ "x${m:0:1}" == "xU" ]]; then
		#tarom="tar60;0"
	#fi

	echo
	echo -n "$m "

	echo -n "om"
	for f in `ls $m/fit/$tarom/coef/`; do
		[[ $f =~ coef[0-9]{8}\.txt ]] && echo -n " ${f:4:8}"
	done
	echo

	echo -n "   tm"
	for f in `ls $m/fit/$tartm/coef/`; do
		[[ $f =~ coef[0-9]{8}\.txt ]] && echo -n " ${f:4:8}"
	done
	echo

	if do_event $m; then
		echo -n "  ome"
		for f in `ls $m/fitTevt/$tarom/coef/`; do
			[[ $f =~ coef[0-9]{8}\.txt ]] && echo -n " ${f:4:8}"
		done
		echo
	fi
done
