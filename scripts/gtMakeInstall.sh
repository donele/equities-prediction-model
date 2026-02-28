#!/bin/bash

#if [[ $HOSTNAME == *"wron-mrct06"* ]]; then

	# CD to build directory.
	buildDir=$HOME/gtdev/build_generaltools
	cd $buildDir || {
	echo "Cannot change to $buildDir"
	exit 1
	}

	# Install destination.
	defaultDir=$HOME/gtbin
	installDir=
	installExt=
	if (( $# == 0 )); then
		installDir=$defaultDir
	elif (( $# == 1 )); then
		installExt="-$1"
		installDir=$defaultDir$installExt
	elif (( $# > 1 )); then
		echo "ERROR: Too many args."
		exit 1
	fi

	# Run make install.
	if [[ "x$installDir" != "x$defaultDir" ]]; then
		echo "make install..."
		tempDir=$defaultDir-`date`
		if [[ -e "$tempDir" ]]; then
			rm -rf "$tempDir"
		fi
		if [[ -e $installDir ]]; then
			rm -rf $installDir
		fi
		mv $defaultDir "$tempDir" && make -j $N_CORES install && mv $defaultDir $installDir && mv "$tempDir" $defaultDir
	else
		echo "make install..."
		make -j $N_CORES install
	fi

	# Check for newer files in the build directory.
	for d in bin lib64; do
		for f in $(ls $installDir/$d); do
			if [ -f $buildDir/$d/$f ]; then
				if [ $buildDir/$d/$f -nt $installDir/$d/$f ]; then
					:
					#echo WARNING: Newer instance of $f is available in $buildDir. Try 'make install'. Aborting. No file has been copied.
					#exit 1
				fi
			fi
		done
	done

	# Copy to remote servers.
	if [[ "x$HOSTNAME" == *"wron-mrct06"* ]]; then
		echo Copy to smrc-chi24:gtbin/ ...
		rsync -auv $HOME/gtbin/ smrc-chi24:gtbin$installExt/
		echo Copy to smrc-chi26:gtbin/ ...
		rsync -auv $HOME/gtbin/ smrc-chi26:gtbin$installExt/
	elif [[ "x$HOSTNAME" == *"smrc-chi24"* ]]; then
		echo Copy to wron-mrct06:gtbin/ ...
		rsync -auv $HOME/gtbin/ wron-mrct06:gtbin$installExt/
		echo Copy to smrc-chi26:gtbin/ ...
		rsync -auv $HOME/gtbin/ smrc-chi26:gtbin$installExt/
	elif [[ "x$HOSTNAME" == *"smrc-chi26"* ]]; then
		echo Copy to wron-mrct06:gtbin/ ...
		rsync -auv $HOME/gtbin/ wron-mrct06:gtbin$installExt/
		echo Copy to smrc-chi24:gtbin/ ...
		rsync -auv $HOME/gtbin/ smrc-chi24:gtbin$installExt/
	fi

#fi
