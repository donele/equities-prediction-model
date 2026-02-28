#!/bin/bash

bdir=$HOME/gtdev/build_generaltools
idir=$HOME/gtbin
for d in bin lib64; do
	for f in $(ls $idir/$d); do
		if [ -f $bdir/$d/$f ]; then
			if [ $bdir/$d/$f -nt $idir/$d/$f ]; then
				echo WARNING: Newer instance of $f is available in $bdir. Try 'make install'. Aborting. No file has been copied.
				exit 1
			fi
		else
			echo WARNING: $f not found in $bdir/$d
		fi
	done
done

echo Copy to smrc-chi22 ...
rsync -auv /home/jelee/gtbin/ mercator1@smrc-chi22:paramjobs/gtbin
