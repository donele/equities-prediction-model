#!/bin/bash
function ts {
  args=$@
  tmux send-keys -t right "$args" C-m
}
function ts0 {
  args=$@
  tmux send-keys -t 0 "$args" C-m
}
function ts1 {
  args=$@
  tmux send-keys -t 1 "$args" C-m
}
function ts2 {
  args=$@
  tmux send-keys -t 2 "$args" C-m
}
function ts3 {
  args=$@
  tmux send-keys -t 3 "$args" C-m
}
function ts4 {
  args=$@
  tmux send-keys -t 4 "$args" C-m
}
function ts5 {
  args=$@
  tmux send-keys -t 5 "$args" C-m
}
function ts6 {
  args=$@
  tmux send-keys -t 6 "$args" C-m
}
function ts7 {
  args=$@
  tmux send-keys -t 7 "$args" C-m
}
function ts8 {
  args=$@
  tmux send-keys -t 8 "$args" C-m
}

# tail -f of the most recent file in the directory.
function ltf {
last_file=`ls -rt *.log *.txt 2> /dev/null | tail -n 1`
echo $ tail -f $last_file
tail -f $last_file
}

# vi the most recent file in the directory.
function ltv {
last_file=`ls -rt 2> /dev/null | tail -n 1`
vi $last_file
}

# Get the list of the log files of the running mx.exe's.
function mlog {
for pid in `ps -e | grep mx | awk '{print $1}'`
do
	ls -l /proc/$pid/fd 2>/dev/null | grep "log" | grep -v "cannot" | grep -v "proc" | grep " 1 ->" | awk "{print $pid,\$NF}; END {if(NR==0) print $pid}"
done
}
alias ml=mlog
function mtail {
# cond: pid or any part of command line
cond=$1
for pid in `ps -ef | grep mx | grep " $cond" | awk '{print $2}'`
do
	f=`ls -l /proc/$pid/fd 2>/dev/null | grep "log" | grep -v "cannot" | grep -v "proc" | grep " 1 ->" | awk '{print $NF}'`
	if [ "x$f" != "x" ] && [ -f $f ]; then
		echo
		echo ">>> Tail of:" $f
		tail $f
	fi
done
}
alias mt=mtail
function mcpu {
pidlist=`ps -ef | grep -v grep | grep mx | awk '{print $2}'`
pidlist=`echo $pidlist | sed 's/ /,/g'`
if [ "x$pidlist" != "x" ]; then
	ps -o pid,pcpu,pmem,args --pid $pidlist --sort pcpu
fi
pidlist=`ps -ef | grep -v grep | grep python | awk '{print $2}'`
pidlist=`echo $pidlist | sed 's/ /,/g'`
if [ "x$pidlist" != "x" ]; then
	ps -o pid,pcpu,pmem,args --pid $pidlist --sort pcpu | awk '{if($2>0)print $0}'
fi
}
function mjob {
nline=$1
pidlist=`ps -ef | grep mx | grep -v gdb | grep -v grep | awk '{print $2}'`
if [ "x$pidlist" != "x" ]; then
	ps -o pid,pcpu,pmem,args | head -n 1
fi
for pid in $pidlist
do
	ps -o pid,pcpu,pmem,args --pid $pid | tail -n 1
	#echo "                cwd:" `readlink /proc/$pid/cwd`
	ls -l /proc/$pid/fd 2>/dev/null | grep "log" | grep -v "cannot" | grep -v "proc" | grep " 1 ->" | awk "{print \"                log:\",\$NF}"
	f=`ls -l /proc/$pid/fd 2>/dev/null | grep "log" | grep -v "cannot" | grep -v "proc" | grep " 1 ->" | awk '{print $NF}'`
	if [ "x$f" != "x" ] && [ -f $f ]; then
		if [ "x$nline" == "x" ]; then
			nline=1
		fi
		echo "               tail:" `tail -n $nline $f`
	fi
done
}
alias mj=mjob
function mtree {
	ps -f --forest | grep -v forest | grep -v grep
}
alias mtr=mtree

# anaoos file
# oosn <ntrees> <file>
function oosn {
ntrees=$1
filename=$2
if grep "mnSp" $ifile > /dev/null 2>&1; then
	awk "{if(\$3==$ntrees) print \$0}" $filename
else
	awk "{if((\$1==\"t\"||\$1==\"w\")&&\$2==$ntrees) print \$0}" $filename
fi
}
# oos <file>
function oos {
for ifile in $@
do
	if [ -e $ifile ]; then
		echo $ifile
	fi
done
for ifile in $@
do
	if [ -e $ifile ]; then
		if grep "mnSp" $ifile > /dev/null 2>&1; then
			ntrees=`awk '{N=$3}; {if(N!="ntree"&&max<N) max=N}; END {print max}' $ifile`
			awk "{if(\$3==$ntrees) print \$0}" $ifile
		else
			ntrees=`awk '{N=$2}; {if(N!="nTree"&&max<N) max=N}; END {print max}' $ifile`
			awk "{if((\$1==\"t\"||\$1==\"w\")&&\$2==$ntrees) print \$0}" $ifile
		fi
	fi
done
}

# Change directory to the next model.
function cdn {
cd `get_next_sig_dir.py`
}
function cdnlt {
cd `get_next_sig_dir.py` && ls -lrt
}

# Find and Replace in file(s).
function myrep {
if (( $# == 2 )); then
	if grep $2 $1 > /dev/null 2>&1; then
		echo "========= Pattern $2 found in files: =========="
		grep $2 $1
	else
		echo "Pattern $2 was not found in $1."
	fi
elif (( $# == 3 )); then
	for f in `grep -l $2 $1 2>/dev/null | grep -v repTry`; do echo; echo "========== $f =========="; sed 's/'"$2"'/'"$3"'/g' $f > ${f}.repTry; diff $f ${f}.repTry; rm ${f}.repTry; done
	echo
	echo -n "Execute the change? [y/N]: "
	read answer
	if [[ "x$answer" == "xy" ]]; then
		echo
		echo "Pattern $2 was replaced by $3 in files:"
		for f in `grep -l $2 $1 2>/dev/null | grep -v repTry`; do echo $f; sed 's/'"$2"'/'"$3"'/g' $f > ${f}.repTry; mv ${f}.repTry $f; done
	else
		echo "Aborted."
	fi
else
	echo "Usage: myrep <file> <pattern>"
	echo "       myrep <file> <pattern> <pattern2>"
fi
}

# Numbered header
function ihead {
head -1 $1 | awk 'BEGIN {RS="\t"}; {print NR-1, $1}'
}

# Change directory to similar directory.
function cdrep {
if (( $# == 2 )); then
	cd ${PWD/$1/$2}
else
	echo "Usage: cdrep pattern1 pattern2"
fi
}

function sortStat {
paste <(head -n 1 stat.txt | awk 'BEGIN{RS=" "};{if($0!="")print ++n,$1}') <(tail -n 1 stat.txt | awk 'BEGIN{RS=" "};{if($0!=""&&$0!="mu")print $1}') | awk '{printf("%2d %13s %5.1f\n",$1,$2,$3)}' | sort -rnk3 | less
}

function showFS {
	maxNetPos=$1
	fname=$2
	for w in 0 0.1 0.3 0.5 0.7; do
		echo "wgts = " $w
		grep "fs_0_${w}_${maxNetPos}_" $fname | grep "|" | grep -v "MFastSim" | grep -v "beginJob" 
	done
}
function showFS2 {
	maxNetPos=$2
	fname=$1
	for w in 0 0.3 0.5 0.7; do
		echo "wgts = " $w
		grep "fs_0_${w}_${maxNetPos}_" $fname | grep "|" | grep -v "MFastSim" | grep -v "beginJob" 
	done
}
