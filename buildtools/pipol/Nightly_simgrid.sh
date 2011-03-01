#!/bin/bash

#PRE-PIPOL /home/mescal/navarro/pre-simgrid.sh

#PIPOL esn i386-linux-ubuntu-karmic.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-ubuntu-karmic.dd.gz none 02:00 --user --silent
#PIPOL esn i386-linux-ubuntu-lucid.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-ubuntu-lucid.dd.gz none 02:00 --user --silent
#PIPOL esn amd64_2010-linux-ubuntu-maverick.dd.gz none 02:00 --user --silent

#PIPOL esn i386-linux-fedora-core12.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-fedora-core12.dd.gz none 02:00 --user --silent
#PIPOL esn i386-linux-fedora-core13.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-fedora-core13.dd.gz none 02:00 --user --silent

#PIPOL esn i386_kvm-linux-debian-lenny none 02:00 --user --silent
#PIPOL esn amd64_kvm-linux-debian-lenny none 02:00 --user --silent
#PIPOL esn i386_kvm-linux-debian-testing none 02:00 --user --silent
#PIPOL esn amd64_kvm-linux-debian-testing none 02:00 --user --silent

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

sh ./buildtools/pipol/liste_install.sh
sh ./buildtools/pipol/install_gtnets.sh ./gtnets_install
SIMGRID_DIR=`pwd`
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$SIMGRID_DIR/gtnets_install/lib
perl ./buildtools/pipol/cmake.pl
perl ./buildtools/pipol/ruby.pl

rm CMakeCache.txt

if [ x$PIPOL_IMAGE == "xamd64_2010-linux-ubuntu-maverick.dd.gz" ] ; then
	#mem-check
	cmake \
	-Denable_lua=off \
	-Denable_tracing=off \
	-Denable_smpi=off \
	-Denable_supernovae=off \
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
	make clean
else
	#supernovae
	cmake \
	-Denable_lua=on \
	-Denable_tracing=on \
	-Denable_smpi=on \
	-Denable_supernovae=on \
	-Denable_compile_optimizations=on \
	-Denable_compile_warnings=on \
	-Denable_lib_static=off \
	-Denable_model-checking=off \
	-Denable_latency_bound_tracking=off \
	-Denable_gtnets=off .
	ctest -D NightlyStart
	ctest -D NightlyConfigure
	ctest -D NightlyBuild
	ctest -D NightlyTest
	ctest -D NightlySubmit
	make clean
	
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
	-Denable_supernovae=off .
	ctest -D NightlyStart
	ctest -D NightlyConfigure
	ctest -D NightlyBuild
	ctest -D NightlyTest
	ctest -D NightlyCoverage
	ctest -D NightlySubmit
	make clean
fi

export SIMGRID_ROOT=`pwd`

cd ../
svn checkout svn://scm.gforge.inria.fr/svn/simgrid/contrib/trunk/simgrid-java simgrid-java --quiet
cd simgrid-java
cmake .
ctest -D NightlyStart
ctest -D NightlyConfigure
ctest -D NightlyBuild
ctest -D NightlyTest
ctest -D NightlySubmit

cd ../
svn checkout svn://scm.gforge.inria.fr/svn/simgrid/contrib/trunk/simgrid-ruby simgrid-ruby --quiet
cd simgrid-ruby
cmake .
ctest -D NightlyStart
ctest -D NightlyConfigure
ctest -D NightlyBuild
ctest -D NightlyTest
ctest -D NightlySubmit