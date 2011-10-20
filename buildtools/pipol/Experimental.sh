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

#Those 3 lines is for GTNetS
#sh ./buildtools/pipol/install_gtnets.sh ./gtnets_install
#SIMGRID_ROOT=`pwd`
#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$SIMGRID_ROOT/gtnets_install/lib

#MC
cmake \
-Denable_latency_bound_tracking=on \
-Denable_gtnets=on \
-Dgtnets_path=./gtnets_install \
-Denable_coverage=on \
-Denable_model-checking=on \
-Denable_compile_optimizations=off \
-Denable_auto_install=on \
-DCMAKE_INSTALL_PREFIX=./simgrid_install \
-Drelease=on .
ctest -D ExperimentalStart
ctest -D ExperimentalConfigure
ctest -D ExperimentalBuild
ctest -D ExperimentalTest
ctest -D ExperimentalCoverage
ctest -D ExperimentalSubmit
make clean
