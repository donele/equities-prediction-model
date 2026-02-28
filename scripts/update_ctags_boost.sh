#!/bin/bash

rpm -ql boost-devel | grep -E -o '/usr/include/.*\.(h|hpp)' | grep -v 'usr/include/boost/typeof/' | grep -v 'usr/include/boost/phoenix/' | grep -v 'usr/include/boost/fusion' > ~/gtdev/tags/boost-filelist
ctags --sort=foldcase --c++-kinds=+p --fields=+iaS --extra=+q -f ~/gtdev/tags/boost.tags -L ~/gtdev/tags/boost-filelist
