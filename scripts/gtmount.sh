#!/bin/bash

if (( $# == 0 )); then
	if [[ "x$HOSTNAME" == *"wron-mrct06"* ]]; then
		sshfs smrc-chi24:gtfit ~/gtfit
		sshfs smrc-chi24:gtrun ~/gtrun
		sshfs smrc-chi24:hffit ~/hffit
		sshfs smrc-chi26:hffit2 ~/hffit2
		sshfs smrc-chi24:pyfit ~/pyfit
	elif [[ "x$HOSTNAME" == *"smrc-chi26"* ]]; then
		sshfs smrc-chi24:gtfit ~/gtfit
		sshfs smrc-chi24:gtrun ~/gtrun
		sshfs smrc-chi24:hffit ~/hffit
		sshfs smrc-chi24:pyfit ~/pyfit
	elif [[ "x$HOSTNAME" == *"smrc-chi24"* ]]; then
		sshfs smrc-chi26:hffit2 ~/hffit2
	fi
elif (( $# == 1 )); then
	if [[ "x$1" == "x-u" ]]; then
		if [[ "x$HOSTNAME" == *"wron-mrct06"* ]]; then
			fusermount -u ~/gtfit
			fusermount -u ~/gtrun
			fusermount -u ~/hffit
			fusermount -u ~/hffit2
		elif [[ "x$HOSTNAME" == *"smrc-chi26"* ]]; then
			fusermount -u ~/gtfit
			fusermount -u ~/gtrun
			fusermount -u ~/hffit
		elif [[ "x$HOSTNAME" == *"smrc-chi24"* ]]; then
			fusermount -u ~/hffit2
		fi
	fi
fi
