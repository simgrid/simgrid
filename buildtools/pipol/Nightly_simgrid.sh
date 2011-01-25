#!/bin/bash

#PRE-PIPOL /home/mescal/navarro/pre-simgrid.sh


#PIPOL esn i386-linux-ubuntu-jaunty.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-ubuntu-jaunty.dd.gz none 02:00 --user --silent
#PIPOL esn i386-linux-ubuntu-karmic.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-ubuntu-karmic.dd.gz none 02:00 --user --silent
#PIPOL esn i386-linux-ubuntu-lucid.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-ubuntu-lucid.dd.gz none 02:00 --user --silent
#PIPOL esn amd64_2010-linux-ubuntu-maverick.dd.gz none 02:00 --user --silent

#PIPOL esn i386-linux-fedora-core11.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-fedora-core11.dd.gz none 02:00 --user --silent
#PIPOL esn i386-linux-fedora-core12.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-fedora-core12.dd.gz none 02:00 --user --silent
#PIPOL esn i386-linux-fedora-core13.dd.gz none 02:00 --user --silent
#PIPOL esn amd64-linux-fedora-core13.dd.gz none 02:00 --user --silent

#PIPOL esn i386_kvm-linux-debian-lenny none 02:00 --user --silent
#PIPOL esn amd64_kvm-linux-debian-lenny none 02:00 --user --silent
#PIPOL esn i386_kvm-linux-debian-testing none 02:00 --user --silent
#PIPOL esn amd64_kvm-linux-debian-testing none 02:00 --user --silent

#PIPOL esn x86_64_mac-mac-osx-server-snow-leopard.dd.gz none 02:00 --user --silent
#PIPOL esn x86_mac-mac-osx-server-snow-leopard.dd.gz none 02:00 --user --silent

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
SIMGRID_ROOT=`pwd`
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$SIMGRID_ROOT/gtnets_install/lib
perl ./buildtools/pipol/cmake.pl
perl ./buildtools/pipol/ruby.pl

rm CMakeCache.txt

#supernovae
cmake \
-Denable_lua=on \
-Denable_ruby=on \
-Denable_java=on \
-Denable_tracing=on \
-Denable_smpi=on \
-Denable_supernovae=on \
-Denable_compile_optimizations=on \
-Denable_compile_warnings=on \
-Denable_lib_static=off \
-Denable_model-checking=off \
-Denable_latency_bound_tracking=off \
-Denable_gtnets=off .
ctest -D ExperimentalStart
ctest -D ExperimentalConfigure
ctest -D ExperimentalBuild
ctest -D ExperimentalTest
ctest -D ExperimentalSubmit
make clean

#MC
cmake \
-Denable_latency_bound_tracking=on \
-Denable_gtnets=on \
-Dgtnets_path=./gtnets_install \
-Denable_coverage=on \
-Denable_model-checking=on \
-Denable_compile_optimizations=off \
-Denable_supernovae=off .
ctest -D ExperimentalStart
ctest -D ExperimentalConfigure
ctest -D ExperimentalBuild
ctest -D ExperimentalTest
ctest -D ExperimentalCoverage
ctest -D ExperimentalSubmit
make clean