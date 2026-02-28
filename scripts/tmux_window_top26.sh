#!/bin/bash

function new_window {
	tmux new-window -n $1
	tmux send-keys -t $1 "ssh smrc-chi26 -t -- htop -u jelee" C-m
}

name=top26
if tmux list-windows | grep -w $name > /dev/null 2>&1; then
	tmux select-window -t $name
else
	new_window $name
fi
