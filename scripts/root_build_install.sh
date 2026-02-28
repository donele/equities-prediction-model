#!/bin/bash
rm -rf root_old 2> /dev/null
mv root root_old 2> /dev/null
git clone http://root.cern.ch/git/root.git && \
cd root &&  \
(
  git checkout -b v6-10-08 v6-10-08 && \
  mkdir builddir
  cd builddir
  cmake ..
  make -j 80
  sudo make install
)
~
