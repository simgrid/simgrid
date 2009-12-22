#!/bin/bash

# This script creates a new dist archive from the svn
source cmake_simgrid.conf

echo "get Linux dependencies"
sudo aptitude install -y libtool automake1.10 autoconf libgcj10-dev gcc g++ bash flex flexml doxygen bibtex bibtool iconv bibtex2html addr2line valgrind

echo "update the svn"
cd ${SIMGRID_SVN_ROOT}
svn up

echo "rebuild the missing files for compilation"
./bootstrap
./configure --enable-maintainer-mode --disable-compile-optimizations

echo "Make the archive"
make all dist

echo "Copy the archive in position"
mv simgrid*.tar.gz ${SIMGRID_BASEDIR}

echo "Done!"
