#!/bin/bash

if [ -e ./pipol ] ; then
	rm -rf ./pipol/$PIPOL_HOST
	mkdir ./pipol/$PIPOL_HOST
else
	mkdir ./pipol
	rm -rf ./pipol/$PIPOL_HOST
	mkdir ./pipol/$PIPOL_HOST
fi
cd ./pipol/$PIPOL_HOST

git clone git://scm.gforge.inria.fr/simgrid/simgrid.git simgrid --quiet
cd simgrid

perl ./buildtools/pipol/cmake.pl
perl ./buildtools/pipol/ruby.pl

if [ -e /usr/bin/gcc-4.6 ] ; then
export CC=gcc-4.6
export CXX=g++-4.6
else
export CC=gcc
export CXX=g++
fi

cmake -Drelease=on .
ctest -D ExperimentalStart
ctest -D ExperimentalConfigure
ctest -D ExperimentalBuild
ctest -D ExperimentalTest
ctest -D ExperimentalSubmit

export SIMGRID_ROOT=`pwd`
export LD_LIBRARY_PATH=`pwd`/lib
export DYLD_LIBRARY_PATH=`pwd`/lib #for mac

cd ../
git clone git://scm.gforge.inria.fr/simgrid/simgrid-java.git simgrid-java --quiet
cd simgrid-java
export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:`pwd`/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`/lib #for mac

cmake .
ctest -D ExperimentalStart
ctest -D ExperimentalConfigure
ctest -D ExperimentalBuild
ctest -D ExperimentalTest
ctest -D ExperimentalSubmit

cd ../
git clone git://scm.gforge.inria.fr/simgrid/simgrid-ruby.git simgrid-ruby --quiet
cd simgrid-ruby

cmake .
ctest -D ExperimentalStart
ctest -D ExperimentalConfigure
ctest -D ExperimentalBuild
ctest -D ExperimentalTest
ctest -D ExperimentalSubmit