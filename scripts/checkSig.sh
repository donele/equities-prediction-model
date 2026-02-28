#!/bin/bash
function do_event {
[[ "x$1" == "xUS" || "x$1" == "xUF" || "x$1" == "xCA" || "x$1" == "xEU" ]]
}

function show_dir {
echo ${1#*/}
ls -lh $1 | tail -5 | awk '{n=length($1)+length($2)+length($3)+length($4)+4}; {t=length($0)}; {print " ", substr($0,n)}'
}

function list_sig {
subdir=$1
echo $subdir -------------------------------------------------------
show_dir $subdir/binSig/om
show_dir $subdir/binSig/tm
if do_event $1; then
	show_dir $subdir/binSigTevt/om
fi
for d in `ls -d $1/filter/*`; do
	show_dir $d
done
echo
}

# Working directory.
dir=$HOME/hffit2
cd $dir || {
echo "Cannot change to $dir" >&2
exit 1
}

if (( $# >= 1 )); then
	for d in "$@"; do
		list_sig $d
	done
else
	for d in `ls -d ??`; do
		list_sig $d
	done
fi

