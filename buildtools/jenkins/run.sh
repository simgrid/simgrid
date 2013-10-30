#!/bin/bash

WORKSPACE=$1
build_mode=$2


rm -rf $WORKSPACE/build

mkdir $WORKSPACE/build
cd $WORKSPACE/build

export PATH=./lib/:../../lib:$PATH

if test "$(uname -o)" = "Msys"
then 
    
    cmake -G "MSYS Makefiles" $WORKSPACE
    #$NUMBER_OF_PROCESSORS should be already set on win
    if [ -z "$NUMBER_OF_PROCESSORS" ]; then
        NUMBER_OF_PROCESSORS=1
    fi  

    if [ $? -ne 0 ] ; then
        echo "Failed to do the first cmake - Halting"
        exit 1
    fi

    make dist

    if [ $? -ne 0 ] ; then
        echo "Failed to build dist - Halting"
        exit 2
    fi

    cmake -G "MSYS Makefiles" -Denable_java=ON -Denable_model-checking=OFF -Denable_lua=OFF -Denable_compile_optimizations=ON  -Denable_smpi=ON -Denable_smpi_MPICH3_testsuite=ON -Denable_compile_warnings=OFF .

    if [ $? -ne 0 ] ; then
        echo "Failed to perform the Cmake for $build_mode - Halting"
        exit 5
    fi

    make -j$NUMBER_OF_PROCESSORS

    if [ $? -ne 0 ] ; then
        echo "Build failure - Halting"
        exit 5
    fi

    make nsis

    if [ $? -ne 0 ] ; then
        echo "Failure while generating the Windows executable - Halting"
        exit 6
    fi

else
    NUMBER_OF_PROCESSORS=0
    # Linux:
    cpuinfo_file="/proc/cpuinfo"
    if [ -f "${cpuinfo_file}" ]; then
      NUMBER_OF_PROCESSORS=$(grep -c "processor.: " ${cpuinfo_file}) 
    fi
    # grep returns 0 or cpuinfo not found
    if [ $NUMBER_OF_PROCESSORS = 0 ]; then
      NUMBER_OF_PROCESSORS=1
    fi

    cmake $WORKSPACE

    if [ $? -ne 0 ] ; then
        echo "Failed to do the first cmake - Halting"
        exit 1
    fi

    rm Simgrid*.tar.gz
    make dist

    if [ $? -ne 0 ] ; then
        echo "Failed to build dist - Halting"
        exit 2
    fi

    tar xzf `cat VERSION`.tar.gz

    if [ $? -ne 0 ] ; then
        echo "Failed to extract the generated tgz - Halting"
        exit 3
    fi

    cd `cat VERSION`

    if [ $? -ne 0 ] ; then
        echo "Path `cat VERSION` cannot be found - Halting"
        exit 4
    fi

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

    if [ $? -ne 0 ] ; then
        echo "Failed to perform the Cmake for $build_mode - Halting"
        exit 5
    fi

    make -j$NUMBER_OF_PROCESSORS

    if [ $? -ne 0 ] ; then
        echo "Build failure - Halting"
        exit 6
    fi

fi


echo "running tests with $NUMBER_OF_PROCESSORS processors"

ctest -T test --no-compress-output  --timeout 100 -j$NUMBER_OF_PROCESSORS || true
if [ -f Testing/TAG ] ; then
   xsltproc $WORKSPACE/buildtools/jenkins/ctest2junit.xsl -o "$WORKSPACE/CTestResults.xml" Testing/`head -n 1 < Testing/TAG`/Test.xml
fi

ctest -D ContinuousStart
ctest -D ContinuousConfigure
ctest -D ContinuousBuild
ctest -D ContinuousTest
ctest -D ContinuousSubmit

rm -rf `cat VERSION`
