#!/bin/bash

#GET the OS name
OS=`uname`
node=`uname -n`

# OS specific working directory 
BASEDIR=/pipol
DIR=$BASEDIR/$OS/$node

# Clean any leftover  from previous install
echo "remove old directory $BASEDIR/$OS/$node"
rm -rf $BASEDIR/$OS/$node

# create a new directory 
echo "create new directory $BASEDIR/$OS/$node"
mkdir  $BASEDIR/$OS
mkdir  $BASEDIR/$OS/$node
cd $BASEDIR/$OS/$node

# load the simgrid directory from svn
echo "load simgrid-svn"
svn checkout svn://scm.gforge.inria.fr/svn/simgrid/simgrid/trunk
mv trunk/ simgrid
cp -r $BASEDIR/$OS/$node/simgrid/buildtools/ctest/Cmake $BASEDIR/$OS/$node/simgrid/Cmake

# Install dependencies
echo "get dependencies"
fink --yes install -y libtool automake1.10 autoconf libgcj10-dev gcc g++ bash flex flexml doxygen bibtex bibtool iconv bibtex2html addr2line valgrind

# 1er test
cd $BASEDIR/$OS/$node/simgrid
echo "./bootstrap"
./bootstrap
echo "./configure"
./configure --enable-maintainer-mode --disable-compile-optimizations
echo "make"
make 
echo "./checkall"
./checkall

# 2eme test ctest
fink --yes install cmake
cd $BASEDIR/$OS/$node/simgrid/Cmake
cmake ./
ctest -D Experimental

echo "Done!"
