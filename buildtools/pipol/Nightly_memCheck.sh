#!/bin/bash

#PIPOL esn amd64_2010-linux-ubuntu-maverick.dd.gz none 02:00 --user --silent

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

perl ./buildtools/pipol/cmake.pl
perl ./buildtools/pipol/ruby.pl

if [ -e /usr/bin/gcc-4.6 ] ; then
export CC=gcc-4.6
export CXX=g++-4.6
else
export CC=gcc
export CXX=g++
fi

#mem-check
cmake \
-Denable_lua=off \
-Denable_tracing=off \
-Denable_smpi=off \
-Denable_compile_optimizations=off \
-Denable_compile_warnings=on \
-Denable_lib_static=off \
-Denable_model-checking=off \
-Denable_latency_bound_tracking=off \
-Denable_gtnets=off \
-Denable_jedule=off \
-Denable_memcheck=on ./
ctest -D NightlyStart
ctest -D NightlyConfigure
ctest -D NightlyBuild
ctest -D NightlyMemCheck
ctest -D NightlySubmit