#!/bin/bash

function usage {
echo "Usage:"
echo "  $ source gtsetup.sh"
echo "  $ source gtsetup.sh test"
}

if [[ "x${0##*/}" == "xgtsetup.sh" ]]; then
	usage
	exit 1
fi

if (( $# == 0 )); then
	export PATH=$HOME/gtbin/bin:$PATH
elif (( $# == 1 )); then
	export PATH=$HOME/gtbin-$1/bin:$PATH
else
	usage
fi
