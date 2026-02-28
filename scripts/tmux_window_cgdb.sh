#!/bin/bash

function new_window {
	tmux new-window -n $1 -c "$HOME/gtfit"
	tmux split-window -h -t ${1}.1 -c "$HOME/gtfit"
	#tmux split-window -v -p 85 -t ${1}.1 -c "$HOME/gtfit"
	#tmux split-window -v -p 85 -t ${1}.2 -c "$HOME/gtfit"

	tmux select-window -t $1
	tmux select-pane -t 1
}

name=cgdb
if tmux list-windows | grep -w $name > /dev/null 2>&1; then
	tmux select-window -t $name
else
	new_window $name
fi
