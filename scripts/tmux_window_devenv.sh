#!/bin/bash

function new_window {
	tmux new-window -n $1 -c "$HOME/gtdev/generaltools/source/src"
	tmux split-window -v -p 13 -t ${1}.1 -c "$HOME/gtdev/build_generaltools_debug"
	tmux split-window -h -p 55 -t ${1}.2 -c "$HOME/gtdev/build_generaltools"
	tmux split-window -h -p 27 -t ${1}.3 -c "$HOME/gtdev/generaltools"

	tmux send-keys -t ${1}.1 'vim -c "NERDTreeToggle"' C-m
	tmux select-pane -t 1
}

name=devenv
if tmux list-windows | grep -w $name > /dev/null 2>&1; then
	tmux select-window -t $name
else
	new_window $name
fi
