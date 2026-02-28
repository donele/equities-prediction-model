#!/bin/bash
BASEDIR=$HOME/gtana/tmux_db
DB=$1
DIR=$BASEDIR/$DB
if [ ! -d "$BASEDIR" ]; then
	mkdir $DIR
fi
if [ ! -d "$DIR" ]; then
	echo $DIR does not exist.
	exit 1
fi

# Write a vim function into a file.
# Following line can be used for html output:
#        silent !cat query.txt | isql $1 mercator1 DBacc101 -b -w > output.html
cat << EOF > $DIR/TmuxSql.vim
function! TmuxSql()
        write! query.txt
        silent !cat query.txt | isql $DB mercator1 DBacc101 -b > output.txt
        silent !tmux send-keys -t :.1 :e C-m
        redraw!
endfunction
nmap <silent> <leader>q :call TmuxSql()<cr>
EOF

# Initialize the input and output files.
> $DIR/query.txt
> $DIR/output.txt
#> $DIR/output.html

# If called inside tmux, create a window.
if [ "$TMUX" != "" ]; then
	if tmux select-window -t $DB > /dev/null 2>&1; then
		:
	else
		tmux new-window -n $DB -c $DIR
		tmux split-window -v -p 15 -t $DB -c $DIR
		tmux send-keys -t ${DB}.1 'vim -R -c "set nowrap" output.txt' C-m
		tmux send-keys -t ${DB}.2 'vim -S TmuxSql.vim -c "set noswapfile" query.txt' C-m i
	fi

# If called outside tmux, create a session.
else
	SESS=db-session-$1
	if tmux has-session -t $SESS > /dev/null 2>&1; then
		echo tmux atttaching to $SESS
		tmux attach -d -t $SESS
	else
		echo tmux creating $SESS
		tmux new-session -d -s $SESS -c $DIR
		tmux rename-window -t $SESS:1 $DB
		tmux splitw -v -p 15 -t $SESS:1 -c $DIR

		tmux send-keys -t ${SESS}:1.1 'vim -R -c "set nowrap" output.txt' C-m
		tmux send-keys -t ${SESS}:1.2 'vim -S TmuxSql.vim -c "set noswapfile" query.txt' C-m i

		tmux select-window -t ${SESS}
		tmux select-pane -t 2
		tmux -2 attach-session -t ${SESS}
	fi
fi
