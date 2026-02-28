#!/bin/sh

if env | grep CYG > /dev/null 2>&1; then
	fromdir=jelee@wron-mrct06:/home/jelee
	dest=$HOME
	rsync -lptgouv $fromdir/{.vimrc,.tmux.conf,.bashrc} $HOME
	rsync -lptgouv $fromdir/scripts/*.sh $HOME/scripts
else
	echo "ERROR: Not a CYGWIN shell."
fi
