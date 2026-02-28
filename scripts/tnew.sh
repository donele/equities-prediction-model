#!/bin/bash

if tmux has-session -t my-session > /dev/null 2>&1; then
	echo tmux attaching to my-session
	tmux attach -d -t my-session

else
	echo tmux creating my-session
	tmux new-session -d -s my-session -c "$HOME"
	tmux rename-window -t my-session:1 git
	tmux split-window -h -t git.1 -c "$HOME"
	tmux split-window -v -t git.2 -c "$HOME"
	tmux send-keys -t git.1 'cd gtdev/generaltools; git status' C-m
	tmux send-keys -t git.2 'ssh smrc-chi24 -t -- "cd gtdev/generaltools; git status; bash --login"' C-m
	tmux send-keys -t git.3 'ssh smrc-chi26 -t -- "cd gtdev/generaltools; git status; bash --login"' C-m

	tmux_window_top.sh
	tmux_window_run24.sh
	tmux_window_run26.sh

	tmux select-window -t git
	tmux select-pane -t 1
	tmux -2 attach-session -t my-session
fi
