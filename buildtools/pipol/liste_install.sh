#!/bin/bash

arch=`uname`

if [ -e /usr/bin/yum ] ; then
	sudo yum -y update
fi

which_cmake=`which cmake`	#cmake necessary
echo $which_cmake
if [ x$which_cmake = x ] ; then
  echo "Try to install cmake"
  if [ -e /usr/bin/apt-get ] ; then
    sudo apt-get -y install cmake
  fi
  if [ -e /usr/bin/yum ] ; then
    sudo yum -y install cmake
  fi
  if [ x$arch = xDarwin ] ; then
    sudo fink -y install cmake
  fi
fi

which_svn=`which svn`	#svn necessary
echo $which_svn
if [ x$which_svn = x ] ; then
  echo "Try to install svn"
  if [ -e /usr/bin/apt-get ] ; then
    sudo apt-get -y install subversion
  fi
  if [ -e /usr/bin/yum ] ; then
    sudo yum -y install subversion
  fi
  if [ x$arch = xDarwin ] ; then
    sudo fink -y install svn
  fi
fi

which_gcc=`which gcc`	#gcc gcc necessary
which_gpp=`which g++`	#gcc g++ necessary
echo $which_gcc
echo $which_gpp
if [ x$which_gcc = x ] ; then
  echo "Try to install gcc"
  if [ -e /usr/bin/apt-get ] ; then
    sudo apt-get -y install gcc g++
  fi
  if [ -e /usr/bin/yum ] ; then
    sudo yum -y install gcc
  fi
  if [ x$arch = xDarwin ] ; then
    sudo fink -y install gcc42
  fi
fi

which_make=`which make`	#make necessary
echo $which_make
if [ x$which_make = x ] ; then
  echo "Try to install make"
  if [ -e /usr/bin/apt-get ] ; then
    sudo apt-get -y install make
  fi
  if [ -e /usr/bin/yum ] ; then
    sudo yum -y install make
  fi
  if [ x$arch = xDarwin ] ; then
    sudo fink -y install make
  fi
fi

which_java=`which java`	#java optional
echo $which_java
if [ x$which_java = x ] ; then
  echo "Try to install java"
  if [ -e /usr/bin/apt-get ] ; then
    sudo apt-get -y install openjdk-6-jdk
  fi
  if [ -e /usr/bin/yum ] ; then
    sudo yum -y install java-1.6.0-openjdk
  fi
  if [ x$arch = xDarwin ] ; then
    sudo fink -y install java-1.6.0-openjdk
  fi
fi

if [ x$arch = xDarwin ] ; then
    which_lua=`find /sw/include -name lualib.h`	#lualib.h optional
else
    which_lua=`find /usr/include/ -name lualib.h`	#lualib.h optional
fi

echo $which_lua
if [ x$which_lua = x ] ; then
  echo "Try to install lua"
  if [ -e /usr/bin/apt-get ] ; then
    sudo apt-get -y install liblua5.1 
  fi
  if [ -e /usr/bin/yum ] ; then
    sudo yum -y install lua-devel
  fi
  if [ x$arch = xDarwin ] ; then
    sudo fink -y install lua51-dev
  fi
fi

if [ x$arch = xDarwin ] ; then
    which_ruby=`find /sw/lib/ruby/ -name ruby.h`	#lualib.h optional
else
    which_ruby=`find /usr/lib/ruby/ -name ruby.h`	#ruby.h optional
fi

echo $which_ruby
if [ x$which_ruby = x ] ; then
  echo "Try to install ruby"
  if [ -e /usr/bin/apt-get ] ; then
    sudo apt-get -y install ruby1.8-dev ruby
  fi
  if [ -e /usr/bin/yum ] ; then
    sudo yum -y install ruby-devel ruby
  fi
  if [ x$arch = xDarwin ] ; then
    sudo fink -y install ruby18-dev ruby
  fi
fi

 which_unzip=`which unzip`	#unzip for gtnets

echo $which_unzip
if [ x$which_unzip = x ] ; then
  echo "Try to install unzip"
  if [ -e /usr/bin/apt-get ] ; then
    sudo apt-get -y install unzip
  fi
  if [ -e /usr/bin/yum ] ; then
    sudo yum -y install unzip
  fi
  if [ x$arch = xDarwin ] ; then
    sudo fink -y install unzip
  fi
fi
