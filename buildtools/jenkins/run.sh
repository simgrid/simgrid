#!/bin/bash

WORKSPACE=$1
build_mode=$2

rm -rf $WORKSPACE/build
rm -rf $WORKSPACE/install

mkdir $WORKSPACE/build
mkdir $WORKSPACE/install
cd $WORKSPACE/build

export PATH=./lib/:../../lib:$PATH

if test "$(uname -o)" = "Msys"
then 
    cmake -G "MSYS Makefiles" -Denable_java=ON -Denable_smpi=ON -Denable_smpi_MPICH3_testsuite=ON -Denable_tracing=ON $WORKSPACE
    make
    make dist
    make nsis
    
    if [ "$build_mode" = "Debug" ]
    then
    cmake -G "MSYS Makefiles" -Denable_coverage=ON -Denable_java=ON -Denable_model-checking=OFF -Denable_lua=OFF -Denable_compile_optimizations=ON  -Denable_smpi=ON -Denable_smpi_MPICH3_testsuite=ON -Denable_compile_warnings=ON .
    fi

    if [ "$build_mode" = "ModelChecker" ]
    then
    cmake -G "MSYS Makefiles" -Denable_coverage=ON -Denable_java=ON -Denable_smpi=ON -Denable_model-checking=ON -Denable_lua=OFF -Denable_compile_optimizations=OFF -Denable_compile_warnings=ON .
    fi

    if [ "$build_mode" = "DynamicAnalysis" ]
    then
    cmake -G "MSYS Makefiles" -Denable_lua=OFF -Denable_java=ON -Denable_tracing=ON -Denable_smpi=ON -Denable_compile_optimizations=OFF -Denable_compile_warnings=ON -Denable_lib_static=OFF -Denable_model-checking=OFF -Denable_latency_bound_tracking=OFF -Denable_gtnets=OFF -Denable_jedule=OFF -Denable_mallocators=OFF -Denable_memcheck=ON .
    fi

    make
else    
    cmake $WORKSPACE
    make dist
    tar xzf `cat $WORKSPACE/VERSION`.tar.gz
    cd `cat VERSION`

    if [ "$build_mode" = "Debug" ]
    then
    cmake -Denable_coverage=ON -Denable_java=ON -Denable_model-checking=OFF -Denable_lua=ON -Denable_compile_optimizations=ON  -Denable_smpi=ON -Denable_smpi_MPICH3_testsuite=ON -Denable_compile_warnings=ON .
    fi

    if [ "$build_mode" = "ModelChecker" ]
    then
    cmake -Denable_coverage=ON -Denable_java=ON -Denable_smpi=ON -Denable_model-checking=ON -Denable_lua=ON -Denable_compile_optimizations=OFF -Denable_compile_warnings=ON .
    fi

    if [ "$build_mode" = "DynamicAnalysis" ]
    then
    cmake -Denable_lua=OFF -Denable_java=ON -Denable_tracing=ON -Denable_smpi=ON -Denable_compile_optimizations=OFF -Denable_compile_warnings=ON -Denable_lib_static=OFF -Denable_model-checking=OFF -Denable_latency_bound_tracking=OFF -Denable_gtnets=OFF -Denable_jedule=OFF -Denable_mallocators=OFF -Denable_memcheck=ON .
    fi

    make
    make dist
fi

ctest -T test --no-compress-output  --timeout 100 || true
if [ -f Testing/TAG ] ; then
   xsltproc $WORKSPACE/buildtools/jenkins/ctest2junit.xsl -o "$WORKSPACE/CTestResults.xml" Testing/`head -n 1 < Testing/TAG`/Test.xml
fi

ctest -D ContinuousStart
ctest -D ContinuousConfigure
ctest -D ContinuousBuild
ctest -D ContinuousTest
ctest -D ContinuousSubmit

rm -rf `cat VERSION`
