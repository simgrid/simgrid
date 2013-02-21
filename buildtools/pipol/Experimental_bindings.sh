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

export GIT_SSL_NO_VERIFY=1
git clone git://scm.gforge.inria.fr/simgrid/simgrid.git
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
ctest -D ExperimentalStart
ctest -D ExperimentalConfigure
ctest -D ExperimentalBuild
ctest -D ExperimentalTest
ctest -D ExperimentalSubmit
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
ctest -D ExperimentalStart
ctest -D ExperimentalConfigure
ctest -D ExperimentalBuild
ctest -D ExperimentalTest
ctest -D ExperimentalCoverage
ctest -D ExperimentalSubmit

export SIMGRID_ROOT=`pwd`
export LD_LIBRARY_PATH=`pwd`/lib
export DYLD_LIBRARY_PATH=`pwd`/lib #for mac

cd ../
git clone git://scm.gforge.inria.fr/simgrid/simgrid-ruby.git simgrid-ruby --quiet
cd simgrid-ruby

cmake .
ctest -D ExperimentalStart
ctest -D ExperimentalConfigure
ctest -D ExperimentalBuild
ctest -D ExperimentalTest
ctest -D ExperimentalSubmit
