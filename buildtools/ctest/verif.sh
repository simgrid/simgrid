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

# load the simgrid directory
echo "untar simgrid-3.3.4-svn.tar.gz"
cp /home/mescal/navarro/simgrid-3.3.4-svn.tar.gz $BASEDIR/$OS/$node/simgrid-3.3.4-svn.tar.gz
tar -xzf ./simgrid-3.3.4-svn.tar.gz
cp /home/mescal/navarro/Cmake.tar.gz $BASEDIR/$OS/$node/simgrid-3.3.4-svn/Cmake.tar.gz
cd $BASEDIR/$OS/$node/simgrid-3.3.4-svn
tar -xzf $BASEDIR/$OS/$node/simgrid-3.3.4-svn/Cmake.tar.gz

# 1er test
echo "./configure"
./configure
echo "make"
make 
echo "./checkall"
./checkall

# 2eme test ctest
sudo aptitude install -y cmake
cd $BASEDIR/$OS/$node/simgrid-3.3.4-svn/Cmake
cmake ./
ctest -D Experimental

echo "Done!"
