#!/bin/sh

if [[ "x$HOSTNAME" == *"wron-mrct06"* ]]; then
	rsync -lptgouv ~/.bashrc ~/.vimrc ~/.tmux.conf ~/.gitconfig ~/.odbc.ini smrc-chi24:
	rsync -lptgouv ~/.bashrc ~/.vimrc ~/.tmux.conf ~/.gitconfig ~/.odbc.ini smrc-chi26:
fi

if [[ "x$HOSTNAME" == *"smrc-chi24"* ]]; then
	rsync -lptgouv ~/.bashrc ~/.vimrc ~/.tmux.conf ~/.gitconfig ~/.odbc.ini wron-mrct06
	rsync -lptgouv ~/.bashrc ~/.vimrc ~/.tmux.conf ~/.gitconfig ~/.odbc.ini smrc-chi26:
fi
