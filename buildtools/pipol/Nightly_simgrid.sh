#!/bin/bash

#PRE-PIPOL /home/mescal/navarro/pre-simgrid.sh

#___________________________________________________________________________________________________
#Ubuntu 9.10________________________________________________________________________________________
#PIPOL esn i386-linux-ubuntu-karmic.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-ubuntu-karmic.dd.gz none 02:00 --user --silent

#Ubuntu 10.04
#PIPOL esn i386-linux-ubuntu-lucid.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-ubuntu-lucid.dd.gz pipol8 05:00 --user --silent

#Ubuntu 10.10
#PIPOL esn amd64_2010-linux-ubuntu-maverick.dd.gz none 02:00 --user --silent

#___________________________________________________________________________________________________
#Fedora 12__________________________________________________________________________________________
#PIPOL esn i386-linux-fedora-core12.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-fedora-core12.dd.gz none 02:00 --user --silent

#Fedora 13
#PIPOL esn i386-linux-fedora-core13.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-fedora-core13.dd.gz none 02:00 --user --silent

#Fedora 14
#PIPOL esn amd64_2010-linux-fedora-core14.dd.gz none 02:00 --user --silent
#PIPOL esn i386_2010-linux-fedora-core14.dd.gz none 02:00 --user --silent

#__________________________________________________________________________________________________
#Debian Lenny 5.0___________________________________________________________________________________
#PIPOL esn i386-linux-debian-lenny.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-debian-lenny.dd.gz none 02:00 --user --silent

#Debian Lenny 6.0
#PIPOL esn amd64_2010-linux-debian-squeeze.dd.gz none 02:00 --user --silent
#PIPOL esn i386_2010-linux-debian-squeeze-navarro-2011-10-03-171100.dd.gz none 02:00 --user --silent

#Debian Testing
#PIPOL esn i386-linux-debian-testing none 02:00 --user --silent
#PIPOL esn amd64-linux-debian-testing none 02:00 --user --silent

#___________________________________________________________________________________________________
#MacOS Snow Leopard 10.6____________________________________________________________________________
#PIPOL esn x86_mac-mac-osx-server-snow-leopard-navarro-2011-09-22-113726.dd.gz none 02:00 --user --silent

if [ -e ./pipol ] ; then
        rm -rf ./pipol/$PIPOL_HOST
        mkdir ./pipol/$PIPOL_HOST
else
        mkdir ./pipol
        rm -rf ./pipol/$PIPOL_HOST
        mkdir ./pipol/$PIPOL_HOST
fi
cd ./pipol/$PIPOL_HOST

export GIT_SSL_NO_VERIFY=1
git clone https://gforge.inria.fr/git/simgrid/simgrid.git
cd simgrid
#git checkout v3_7_x

perl ./buildtools/pipol/cmake.pl
perl ./buildtools/pipol/ruby.pl

if [ -e /usr/bin/gcc-4.6 ] ; then
export CC=gcc-4.6
export CXX=g++-4.6
else
export CC=gcc
export CXX=g++
fi

mkdir build-def
cd build-def

#DEFAULT CONF
cmake ..
ctest -D NightlyStart
ctest -D NightlyConfigure
ctest -D NightlyBuild
ctest -D NightlyTest
ctest -D NightlySubmit
cd ../
rm -rf ./build-def

# really clean the working directory
git reset --hard master
git clean -dfx

#MC
cmake \
-Denable_coverage=on \
-Denable_model-checking=on \
-Denable_lua=on \
-Denable_compile_optimizations=off .
ctest -D NightlyStart
ctest -D NightlyConfigure
ctest -D NightlyBuild
ctest -D NightlyTest
ctest -D NightlyCoverage
ctest -D NightlySubmit

export SIMGRID_ROOT=`pwd`
export LD_LIBRARY_PATH=`pwd`/lib
export DYLD_LIBRARY_PATH=`pwd`/lib #for mac
cd ..

git clone git://scm.gforge.inria.fr/simgrid/simgrid-java.git simgrid-java --quiet
cd simgrid-java
export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:`pwd`/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`/lib #for mac

cmake .
ctest -D NightlyStart
ctest -D NightlyConfigure
ctest -D NightlyBuild
ctest -D NightlyTest
ctest -D NightlySubmit

cd ../
git clone git://scm.gforge.inria.fr/simgrid/simgrid-ruby.git simgrid-ruby --quiet
cd simgrid-ruby

cmake .
ctest -D NightlyStart
ctest -D NightlyConfigure
ctest -D NightlyBuild
ctest -D NightlyTest
ctest -D NightlySubmit
