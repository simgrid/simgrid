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

rm CMakeCache.txt

#ucontext and pthread
cmake -Denable_lua=on \
-Denable_ruby=on \
-Denable_lib_static=on \
-Denable_model-checking=off \
-Denable_tracing=on \
-Denable_latency_bound_tracking=off \
-Denable_gtnets=off \
-Denable_java=on \
-Denable_compile_optimizations=off \
-Denable_compile_warnings=off \
-Denable_supernovae=off \
-Denable_smpi=on .
ctest -D ExperimentalStart
ctest -D ExperimentalConfigure
ctest -D ExperimentalBuild
ctest -D ExperimentalTest
ctest -D ExperimentalSubmit
make clean

#supernovae
cmake -Denable_lua=on \
-Denable_ruby=on \
-Denable_lib_static=on \
-Denable_model-checking=off \
-Denable_tracing=on \
-Denable_latency_bound_tracking=on \
-Denable_gtnets=off \
-Denable_java=on \
-Denable_compile_optimizations=off \
-Denable_compile_warnings=off \
-Denable_supernovae=on \
-Denable_smpi=on .
ctest -D ExperimentalStart
ctest -D ExperimentalConfigure
ctest -D ExperimentalBuild
ctest -D ExperimentalTest
ctest -D ExperimentalSubmit
make clean

#model checking
cmake -Denable_lua=on \
-Denable_ruby=on \
-Denable_lib_static=on \
-Denable_model-checking=on \
-Denable_tracing=on \
-Denable_latency_bound_tracking=on \
-Denable_gtnets=off \
-Denable_java=on \
-Denable_compile_optimizations=off \
-Denable_compile_warnings=off \
-Denable_supernovae=off \
-Denable_coverage=on \
-Denable_smpi=on .
ctest -D ExperimentalStart
ctest -D ExperimentalConfigure
ctest -D ExperimentalBuild
ctest -D ExperimentalTest
ctest -D ExperimentalCoverage
ctest -D ExperimentalSubmit
make clean

if [ $SYSTEM = Linux ] ; then

sh ./buildtools/pipol/install_gtnets.sh ./gtnets_install/
	
	if [ -e ./gtnets_install/lib/libgtsim-opt.so ] ; then
		#gtnets
		cmake -Denable_lua=on \
		-Denable_ruby=on \
		-Denable_lib_static=on \
		-Denable_model-checking=off \
		-Denable_tracing=on \
		-Denable_latency_bound_tracking=on \
		-Denable_gtnets=on \
		-Dgtnets_path=./gtnets_install/ \
		-Denable_java=on \
		-Denable_coverage=off \
		-Denable_smpi=on .
		ctest -D ExperimentalStart
		ctest -D ExperimentalConfigure
		ctest -D ExperimentalBuild
		ctest -D ExperimentalTest
		ctest -D ExperimentalSubmit
		make clean
	fi
	
	#full_flags
	cmake -Denable_lua=on \
	-Denable_ruby=on \
	-Denable_lib_static=on \
	-Denable_model-checking=off \
	-Denable_tracing=on \
	-Denable_latency_bound_tracking=on \
	-Denable_gtnets=off \
	-Denable_java=on \
	-Denable_compile_optimizations=on \
	-Denable_compile_warnings=on \
	-Denable_smpi=on .
	ctest -D ExperimentalStart
	ctest -D ExperimentalConfigure
	ctest -D ExperimentalBuild
	ctest -D ExperimentalTest
	ctest -D ExperimentalSubmit
	make clean
fi