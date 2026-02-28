#!/bin/bash

# Copies the scripts to the local copy of the repository where the changes can be committed.

dest=$HOME/gtdev/generaltools/scripts
scripts="$HOME/.bash_profile $HOME/.bashrc $HOME/.vimrc $HOME/.tmux.conf $HOME/.cgdb/cgdbrc $HOME/.gitconfig $HOME/.rootrc $HOME/.odbc.ini"
for s in $scripts; do
	#rsync -lptgouv $s $dest
	cp $s $dest
done
