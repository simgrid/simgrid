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

svn checkout svn://scm.gforge.inria.fr/svn/simgrid/simgrid/trunk simgrid-trunk --quiet
cd simgrid-trunk

perl ./buildtools/pipol/cmake.pl
perl ./buildtools/pipol/ruby.pl

#supernovae
cmake .
ctest -D ExperimentalStart
ctest -D ExperimentalConfigure
ctest -D ExperimentalBuild
ctest -D ExperimentalTest
ctest -D ExperimentalSubmit

export SIMGRID_ROOT=`pwd`
export LD_LIBRARY_PATH=`pwd`/lib

cd ../
svn checkout svn://scm.gforge.inria.fr/svn/simgrid/contrib/trunk/simgrid-java simgrid-java --quiet
cd simgrid-java
cmake .
ctest -D ExperimentalStart
ctest -D ExperimentalConfigure
ctest -D ExperimentalBuild
ctest -D ExperimentalTest
ctest -D ExperimentalSubmit

cd ../
svn checkout svn://scm.gforge.inria.fr/svn/simgrid/contrib/trunk/simgrid-ruby simgrid-ruby --quiet
cd simgrid-ruby
cmake .
ctest -D ExperimentalStart
ctest -D ExperimentalConfigure
ctest -D ExperimentalBuild
ctest -D ExperimentalTest
ctest -D ExperimentalSubmit