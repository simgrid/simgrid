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
mkdir  $BASEDIR/$OS/$node
cd $BASEDIR/$OS/$node

# load the simgrid directory
echo "untar simgrid-3.3.4-svn.tar.gz"
cp /home/mescal/navarro/simgrid-3.3.4-svn.tar.gz $BASEDIR/$OS/$node/simgrid-3.3.4-svn.tar.gz
tar -xzf ./simgrid-3.3.4-svn.tar.gz
echo "untar Cmake.tar.gz"
cp /home/mescal/navarro/Cmake.tar.gz $BASEDIR/$OS/$node/simgrid-3.3.4-svn/Cmake.tar.gz
cd $BASEDIR/$OS/$node/simgrid-3.3.4-svn
tar -xzf $BASEDIR/$OS/$node/simgrid-3.3.4-svn/Cmake.tar.gz

# install cmake
sudo aptitude install -y cmake

# 1er test
cd $BASEDIR/$OS/$node/simgrid-3.3.4-svn
echo "make clean"
make clean
echo "./configure"
./configure
echo "make"
make 

cd $BASEDIR/$OS/$node/simgrid-3.3.4-svn/Cmake
cmake ./
ctest -D Experimental


# 2eme test supernovae
cd $BASEDIR/$OS/$node/simgrid-3.3.4-svn
echo "make clean"
make clean
echo "./configure --enable-supernovae"
./configure --enable-supernovae
echo "make"
make 

cd $BASEDIR/$OS/$node/simgrid-3.3.4-svn/Cmake
cmake ./
ctest -D Experimental

# 3eme test pthread
cd $BASEDIR/$OS/$node/simgrid-3.3.4-svn
echo "make clean"
make clean
echo "./configure --with-pthread"
./configure --with-pthread
echo "make"
make 

cd $BASEDIR/$OS/$node/simgrid-3.3.4-svn/Cmake
cmake ./
ctest -D Experimental

# 4eme test disable compile optimizations
cd $BASEDIR/$OS/$node/simgrid-3.3.4-svn
echo "make clean"
make clean
echo "./configure --disable-compile-optimizations"
./configure --disable-compile-optimizations
echo "make"
make 

cd $BASEDIR/$OS/$node/simgrid-3.3.4-svn/Cmake
cmake ./
ctest -D Experimental

echo "Done!"
