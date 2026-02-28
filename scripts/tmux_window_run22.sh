#!/bin/bash

function new_window {
	tmux new-window -n $1
	tmux split-window -v -t $1

	tmux split-window -h -t ${1}.1
	tmux split-window -h -t ${1}.2

	tmux send-keys -t ${1}.1 'ssh mercator1@smrc-chi22' C-m
	tmux send-keys -t ${1}.3 'ssh mercator1@smrc-chi22' C-m
	tmux send-keys -t ${1}.2 'ssh mercator1@smrc-chi22' C-m
	tmux send-keys -t ${1}.4 'ssh mercator1@smrc-chi22' C-m

	tmux select-pane -t 3
}

name=chi22
if tmux list-windows | grep -w $name > /dev/null 2>&1; then
	tmux select-window -t $name
else
	new_window $name
fi
