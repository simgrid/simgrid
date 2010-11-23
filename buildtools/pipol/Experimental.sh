#!/bin/bash

SYSTEM=`uname`

if [ -e ./pipol ] ; then
	rm -rf ./pipol/$PIPOL_HOST
	mkdir ./pipol/$PIPOL_HOST
else
	mkdir ./pipol
	rm -rf ./pipol/$PIPOL_HOST
	mkdir ./pipol/$PIPOL_HOST
fi
cd ./pipol/$PIPOL_HOST

svn checkout svn://scm.gforge.inria.fr/svn/simgrid/simgrid/trunk simgrid-trunk --quiet

sh ./simgrid-trunk/buildtools/pipol/liste_install.sh
perl ./simgrid-trunk/buildtools/pipol/cmake.pl

cd simgrid-trunk
mkdir tmp_build
cd tmp_build

cmake -Denable_lua=on -Denable_ruby=on -Denable_lib_static=on -Denable_graphviz=on -Denable_model-checking=on -Denable_tracing=on -Denable_latency_bound_tracking=on ..

ctest -D ExperimentalStart
ctest -D ExperimentalConfigure
ctest -D ExperimentalBuild
ctest -D ExperimentalTest
ctest -D ExperimentalSubmit
make clean

rm -rf *
cmake -Denable_lua=on -Denable_ruby=on -Denable_lib_static=on -Denable_graphviz=on -Denable_model-checking=on -Denable_tracing=on -Denable_latency_bound_tracking=on -Dwith_context=pthread ..
ctest -D ExperimentalStart
ctest -D ExperimentalConfigure
ctest -D ExperimentalBuild
ctest -D ExperimentalTest
ctest -D ExperimentalSubmit