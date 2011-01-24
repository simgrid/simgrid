#!/bin/bash

arch=`uname`

if [ -e /usr/bin/apt-get ] ; then
    sudo apt-get -y -qq install subversion
    sudo apt-get -y -qq install gcc
    sudo apt-get -y -qq install g++
    sudo apt-get -y -qq install make
    sudo apt-get -y -qq install openjdk-6-jdk
    sudo apt-get -y -qq install liblua5.1-dev lua5.1
    sudo apt-get -y -qq install unzip
    sudo apt-get -y -qq install cmake
    sudo apt-get -y -qq install wget
    sudo apt-get -y -qq install perl
    sudo apt-get -y -qq install graphviz-dev graphviz
    sudo apt-get -y -qq install libpcre3-dev
    sudo apt-get -y -qq install f2c
   	sudo apt-get -y -qq install valgrind
else
	if [ -e /usr/bin/yum ] ; then
		sudo yum -y -q update
	    sudo yum -y -q install subversion
	    sudo yum -y -q install gcc
	    sudo yum -y -q install make
	    sudo yum -y -q install java-1.6.0-openjdk
	    sudo yum -y -q install lua-devel lua
	    sudo yum -y -q install unzip
	    sudo yum -y -q install cmake
	    sudo yum -y -q install wget
	    sudo yum -y -q install perl
	    sudo yum -y -q install graphviz-dev graphviz
	    sudo yum -y -q install libpcre3-dev
	    sudo yum -y -q install f2c
	else
		if [ x$arch = xDarwin ] ; then
			sudo fink -y -q -b selfupdate
		    sudo fink -y -q -b install svn
		    sudo fink -y -q -b install gcc42
		    sudo fink -y -q -b install make
		    sudo fink -y -q -b install lua51-dev lua51
		    sudo fink -y -q -b install unzip
		    sudo fink -y -q -b install cmake
		    sudo fink -y -q -b install wget
		    sudo fink -y -q -b install perl
		    sudo fink -y -q -b install gd2 graphviz graphviz-dev
		    sudo fink -y -q -b install pcre
		    sudo fink -y -q -b install f2c
		fi
	fi
fi

which_svn=`which svn`		#svn necessary
which_gcc=`which gcc`		#gcc gcc necessary
which_gpp=`which g++`		#gcc g++ necessary
which_make=`which make`		#make necessary
which_java=`which java`		#java optional
which_lua=`which lua`		#lua
which_cmake=`which cmake`	#cmake necessary
which_unzip=`which unzip`	#unzip for gtnets
which_wget=`which wget`		#wget for cmake
which_dot=`which dot`		#dot for cgraph
which_perl=`which perl`		#perl
which_f2c=`which f2c`		#f2c
which_gcov=`which gcov`     #gcov
echo "DEBUT----------------------------------"
echo $which_cmake
echo $which_unzip
echo $which_lua
echo $which_java
echo $which_make
echo $which_gcc
echo $which_gpp
echo $which_svn
echo $which_dot
echo $which_wget
echo $which_perl
echo $which_f2c
echo $which_gcov
echo "FIN------------------------------------"