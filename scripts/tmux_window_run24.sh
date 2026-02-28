#!/bin/bash

function new_window {
	tmux new-window -n $1
	tmux split-window -h -t $1
	tmux send-keys -t ${1}.2 'ssh smrc-chi24 -t -- "tmux ls 2> /dev/null; bash --login"' C-m
	tmux send-keys -t ${1}.1 "cd $HOME/hffit" C-m C-l

	tmux select-pane -t 2
}

function get_window_name {
base=$1
name=${base}
cnt=2
while tmux list-windows | grep -w $name > /dev/null 2>&1; do
	name="${base}-${cnt}"
	(( ++cnt ))
done
}

get_window_name "run24"
new_window $name
