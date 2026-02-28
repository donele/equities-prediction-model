#!/bin/bash

function new_window {
	tmux new-window -n $1
	tmux split-window -v
	tmux send-keys -t ${1}.1 "ssh smrc-chi24 -t -- htop -u jelee" C-m
	tmux send-keys -t ${1}.2 "ssh smrc-chi26 -t -- htop -u jelee" C-m
}

name=top
if tmux list-windows | grep -w $name > /dev/null 2>&1; then
	tmux select-window -t $name
else
	new_window $name
fi
