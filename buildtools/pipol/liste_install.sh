#!/bin/bash

arch=`uname`

if [ -e /usr/bin/apt-get ] ; then
    sudo apt-get -y install subversion
    sudo apt-get -y install gcc
    sudo apt-get -y install g++
    sudo apt-get -y install make
    sudo apt-get -y install openjdk-6-jdk
    sudo apt-get -y install liblua5.1-dev lua5.1
    sudo apt-get -y install ruby1.8-dev ruby
    sudo apt-get -y install unzip
    sudo apt-get -y install cmake
    sudo apt-get -y install wget
    sudo apt-get -y install perl
    sudo apt-get -y install graphviz-dev graphviz
fi

if [ -e /usr/bin/yum ] ; then
	sudo yum -y update
    sudo yum -y install subversion
    sudo yum -y install gcc
    sudo yum -y install make
    sudo yum -y install java-1.6.0-openjdk
    sudo yum -y install lua-devel lua
    sudo yum -y install ruby-devel ruby
    sudo yum -y install unzip
    sudo yum -y install cmake
    sudo yum -y install wget
    sudo yum -y install perl
    sudo yum -y install graphviz-dev graphviz
fi

if [ x$arch = xDarwin ] ; then
    sudo fink -y install svn
    sudo fink -y install gcc42
    sudo fink -y install make
    sudo fink -y install java-1.6.0-openjdk
    sudo fink -y install lua51-dev lua51
    sudo fink -y install ruby18-dev ruby
    sudo fink -y install unzip
    sudo fink -y install cmake
    sudo fink -y install wget
    sudo fink -y install perl
    sudo fink -y install graphviz-dev graphviz
fi

which_svn=`which svn`		#svn necessary
which_gcc=`which gcc`		#gcc gcc necessary
which_gpp=`which g++`		#gcc g++ necessary
which_make=`which make`		#make necessary
which_java=`which java`		#java optional
which_lua=`which lua`		#lua
which_ruby=`which ruby`		#ruby
which_cmake=`which cmake`	#cmake necessary
which_unzip=`which unzip`	#unzip for gtnets
which_wget=`which wget`		#wget for cmake
which_dot=`which dot`		#dot for cgraph
echo $which_cmake
echo $which_unzip
echo $which_ruby
echo $which_lua
echo $which_java
echo $which_make
echo $which_gcc
echo $which_gpp
echo $which_svn
echo $which_dot
echo $which_wget