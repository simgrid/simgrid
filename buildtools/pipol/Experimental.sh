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

if [ x$PIPOL_IMAGE == "xamd64-windows-server-2008-64bits.dd.gz" ] ; then

	export PATH=/cygdrive/c/:/cygdrive/c/GnuWin32/bin/:/cygdrive/c/Windows/system32:/cygdrive/c/Windows
	export PATH=$PATH:/cygdrive/c/CMake\ 2.8/bin/:/cygdrive/c/strawberry/c/bin:/cygdrive/c/strawberry/perl/site/bin:/cygdrive/c/strawberry/perl/bin
	
	cmake \
	-G"Unix Makefiles" \
	-Denable_lua=off \
	-Denable_tracing=off \
	-Denable_smpi=off \
	-Denable_supernovae=off \
	-Denable_compile_optimizations=off \
	-Denable_compile_warnings=off \
	-Denable_lib_static=off \
	-Denable_model-checking=off \
	-Denable_latency_bound_tracking=off \
	-Denable_gtnets=off .
	ctest -D ExperimentalStart
	ctest -D ExperimentalConfigure
	ctest -D ExperimentalBuild
	ctest -D ExperimentalSubmit
else

	perl ./buildtools/pipol/cmake.pl
	perl ./buildtools/pipol/ruby.pl
		
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
		ctest -D ExperimentalStart
		ctest -D ExperimentalConfigure
		ctest -D ExperimentalBuild
		ctest -D ExperimentalMemCheck
		ctest -D ExperimentalSubmit
	else
		sh ./buildtools/pipol/install_gtnets.sh ./gtnets_install
		SIMGRID_ROOT=`pwd`
		export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$SIMGRID_ROOT/gtnets_install/lib

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
		-Denable_auto_install=on \
		-DCMAKE_INSTALL_PREFIX=./simgrid_install \
		-Denable_supernovae=off .
		ctest -D ExperimentalStart
		ctest -D ExperimentalConfigure
		ctest -D ExperimentalBuild
		ctest -D ExperimentalTest
		ctest -D ExperimentalCoverage
		ctest -D ExperimentalSubmit
		make clean
	fi
fi
