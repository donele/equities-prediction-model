#!/bin/bash

ctags -R -f /home/jelee/gtdev/tags/generaltools.tags /home/jelee/gtdev/generaltools/source

# Cscope
cd $HOME/gtdev
find generaltools/source -name '*.cpp' -o -name '*.h' > cscope.files
cscope -q -R -b -i cscope.files
