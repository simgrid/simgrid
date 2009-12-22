#!/bin/bash

#PIPOL esn i386-linux-ubuntu-jaunty.dd.gz none 02:00 --user --silent
#PIPOL esn i386-linux-ubuntu-karmic.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-ubuntu-jaunty.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-ubuntu-karmic.dd.gz none 02:00 --user --silent 

#PIPOL esn i386-linux-debian-etch.dd.gz none 02:00 --user --silent
#PIPOL esn i386-linux-debian-lenny.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-debian-etch.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-debian-lenny.dd.gz none 02:00 --user --silent

#PIPOL esn i386-linux-fedora-core11.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-fedora-core11.dd.gz none 02:00 --user --silent

#PIPOL esn i386-linux-mandriva-2009_powerpack.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-mandriva-2009_powerpack.dd.gz none 02:00 --user --silent

#PIPOL esn i386_mac-mac-osx-server-leopard.dd.gz none 02:00 --user --silent

#GET the OS name
OS=`uname`
node=`uname -n`

# OS specific working directory 
BASEDIR=/pipol
DIR=$BASEDIR/$OS/$node

# Clean any leftover from previous install
echo "remove old directory $BASEDIR/$OS/$node"
rm -rf $BASEDIR/$OS/$node

# create a new directory 
echo "create new directory $BASEDIR/$OS/$node"
mkdir  $BASEDIR/$OS
mkdir  $BASEDIR/$OS/$node
cd $BASEDIR/$OS/$node

echo "WAIT"
echo "attente de distrib"
while [ ! -e $BASEDIR/simgrid*.tar.gz ]; 
do
	wait 1
	#nothing to do except waiting
done
echo "distrib disponible"

# recuperation de la distrib
cp $BASEDIR/simgrid*.tar.gz $BASEDIR/$OS/$node/simgrid*.tar.gz
cd $BASEDIR/$OS/$node

# untar de la distrib
tar xzvf ./simgrid*.tar.gz
rm simgrid*.tar.gz

# copie de Cmake
cp -r $BASEDIR/Cmake $BASEDIR/$OS/$node/simgrid/Cmake

# ./configure
cd $BASEDIR/$OS/$node/simgrid
./configure

# make
echo "make"
make 

# 1er test
echo "./checkall"
./checkall

# 2eme test ctest
sudo aptitude install -y cmake
cd $BASEDIR/$OS/$node/simgrid/Cmake
cmake ./
ctest -D Experimental CTEST_FULL_OUTPUT

echo "Done!"
