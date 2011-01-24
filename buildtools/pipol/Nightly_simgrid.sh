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
perl ./simgrid-trunk/buildtools/pipol/ruby.pl

cd simgrid-trunk

rm CMakeCache.txt

cmake \
-Denable_lua=on \
-Denable_ruby=on \
-Denable_java=on \
-Denable_tracing=on \
-Denable_smpi=on \
-Denable_lib_static=off \
-Denable_model-checking=off \
-Denable_latency_bound_tracking=off \
-Denable_gtnets=off \
-Denable_compile_optimizations=off \
-Denable_compile_warnings=off \
-Denable_supernovae=off .
ctest -D NightlyStart
ctest -D NightlyConfigure
ctest -D NightlyBuild
ctest -D NightlyTest
ctest -D NightlySubmit
make clean

#full_flags
cmake \
-Denable_lua=on \
-Denable_ruby=on \
-Denable_java=on \
-Denable_tracing=on \
-Denable_smpi=on \
-Denable_compile_optimizations=on \
-Denable_compile_warnings=on \
-Denable_lib_static=off \
-Denable_model-checking=off \
-Denable_latency_bound_tracking=off \
-Denable_gtnets=off \
-Denable_supernovae=off .
ctest -D NightlyStart
ctest -D NightlyConfigure
ctest -D NightlyBuild
ctest -D NightlyTest
ctest -D NightlySubmit
make clean

#supernovae
cmake \
-Denable_lua=on \
-Denable_ruby=on \
-Denable_java=on \
-Denable_tracing=on \
-Denable_smpi=on \
-Denable_supernovae=on \
-Denable_compile_optimizations=off \
-Denable_compile_warnings=off \
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

#model checking
cmake \
-Denable_coverage=on \
-Denable_lua=on \
-Denable_ruby=on \
-Denable_java=on \
-Denable_model-checking=on \
-Denable_tracing=on \
-Denable_latency_bound_tracking=on \
-Denable_gtnets=off \
-Denable_compile_optimizations=off \
-Denable_compile_warnings=off \
-Denable_supernovae=off \
-Denable_lib_static=off \
-Denable_smpi=on .
ctest -D NightlyStart
ctest -D NightlyConfigure
ctest -D NightlyBuild
ctest -D NightlyTest
ctest -D NightlyCoverage
ctest -D NightlySubmit
make clean

if [ $SYSTEM = Linux ] ; then

	sh ./buildtools/pipol/install_gtnets.sh ./gtnets_install
	
	if [ -e ./gtnets_install/lib/libgtsim-opt.so ] ; then
		#gtnets
		cmake -Denable_lua=on \
		-Denable_ruby=on \
		-Denable_lib_static=on \
		-Denable_model-checking=off \
		-Denable_tracing=on \
		-Denable_latency_bound_tracking=on \
		-Denable_gtnets=on \
		-Denable_java=on \
		-Dgtnets_path=$userhome/usr \
		-Denable_coverage=off \
		-Denable_smpi=on .
		ctest -D NightlyStart
		ctest -D NightlyConfigure
		ctest -D NightlyBuild
		ctest -D NightlyTest
		ctest -D NightlySubmit
		make clean
	fi

	#Make the memcheck mode
	cmake -Denable_lua=off \
	-Denable_ruby=off \
	-Denable_lib_static=off \
	-Denable_model-checking=off \
	-Denable_tracing=off \
	-Denable_latency_bound_tracking=off \
	-Denable_coverage=off \
	-Denable_gtnets=off \
	-Denable_java=off \
	-Denable_memcheck=on .
	ctest -D NightlyStart
	ctest -D NightlyConfigure
	ctest -D NightlyBuild
	ctest -D NightlyMemCheck
	ctest -D NightlySubmit
	make clean
	
fi