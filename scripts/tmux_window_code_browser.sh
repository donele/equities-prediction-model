#!/bin/bash

function new_window {
	tmux new-window -n $1 -c "$HOME/gtdev/generaltools/source/src"
	tmux send-keys -t $1 'vim -c "NERDTreeToggle"' C-m
	tmux select-window -t $1
}

name=code
if tmux list-windows | grep -w $name > /dev/null 2>&1; then
	tmux select-window -t $name
else
	new_window $name
fi
