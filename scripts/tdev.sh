#!/bin/bash

if tmux has-session -t dev-session > /dev/null 2>&1; then
	echo tmux attaching to dev-session
	tmux attach -d -t dev-session

else
	echo tmux creating dev-session
	tmux new-session -d -s dev-session

	tmux rename-window -t dev-session:1 V
	tmux split-window -h -t dev-session:1
	tmux send-keys -t V.1 'cd ~' C-m C-l
	tmux send-keys -t V.2 'cd ~' C-m C-l

	tmux_window_devenv.sh
	tmux_window_cgdb.sh

	tmux select-window -t dev-session:1
	tmux select-pane -t 1
	tmux -2 attach-session -t dev-session
fi
