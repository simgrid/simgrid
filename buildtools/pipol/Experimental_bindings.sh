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

#supernovae
if [ "x$PIPOL_IMAGE" = "xi386-linux-debian-testing.dd.gz" ] ; then
	cmake -DCMAKE_C_COMPILER=gcc-4.6 -DCMAKE_CXX_COMPILER=g++-4.6 -Drelease=on .
else
	cmake -Drelease=on .
fi
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

if [ "x$PIPOL_IMAGE" = "xi386-linux-debian-testing.dd.gz" ] ; then
	cmake -DCMAKE_C_COMPILER=gcc-4.6 -DCMAKE_CXX_COMPILER=g++-4.6 .
else
	cmake .
fi
ctest -D ExperimentalStart
ctest -D ExperimentalConfigure
ctest -D ExperimentalBuild
ctest -D ExperimentalTest
ctest -D ExperimentalSubmit

cd ../
svn checkout svn://scm.gforge.inria.fr/svn/simgrid/contrib/trunk/simgrid-ruby simgrid-ruby --quiet
cd simgrid-ruby

if [ "x$PIPOL_IMAGE" = "xi386-linux-debian-testing.dd.gz" ] ; then
	cmake -DCMAKE_C_COMPILER=gcc-4.6 -DCMAKE_CXX_COMPILER=g++-4.6 .
else
	cmake .
fi
ctest -D ExperimentalStart
ctest -D ExperimentalConfigure
ctest -D ExperimentalBuild
ctest -D ExperimentalTest
ctest -D ExperimentalSubmit