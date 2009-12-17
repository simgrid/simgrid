#! /bin/sh

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



if [[ $OS == 'Linux' ]]; then
	# Install dependencies Linux
	echo "get dependencies Linux"
	sudo aptitude install -y libtool automake1.10 autoconf libgcj10-dev gcc g++ bash flex flexml doxygen bibtex bibtool iconv bibtex2html addr2line valgrind
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
	sudo aptitude install -y cmake
	cd $BASEDIR/$OS/$node/simgrid/Cmake
	cmake ./
	ctest -D Nightly
fi

if [[ $OS == 'Darwin' ]]; then
	# Install dependencies Mac
	echo "get dependencies Mac"
	fink --yes install libtool14
        # fink --yes install doxygen 
        fink --yes install autoconf
	# 1er test
	cd $BASEDIR/$OS/$node/simgrid
	echo "./bootstrap"
	./bootstrap
	echo "./configure"
	./configure --enable-maintainer-mode MAKE=gmake --disable-compile-optimizations
	echo "make"
	make 
	echo "./checkall"
	./checkall
	# 2eme test ctest
	fink --yes install cmake
	cd $BASEDIR/$OS/$node/simgrid/Cmake
	cmake ./
	ctest -D Nightly
fi

echo "Finish !"
