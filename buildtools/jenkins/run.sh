#!/bin/bash

WORKSPACE=$1
build_mode=$2

# usage: die status message...
die () {
  local status=${1:-1}
  shift
  [ $# -gt 0 ] || set -- "Error - Halting"
  echo "$@" >&2
  exit $status
}

rm -rf $WORKSPACE/build

mkdir $WORKSPACE/build
cd $WORKSPACE/build

export PATH=./lib/:../../lib:$PATH

if test "$(uname -o)" = "Msys"
then 
    #$NUMBER_OF_PROCESSORS should be already set on win
    if [ -z "$NUMBER_OF_PROCESSORS" ]; then
        NUMBER_OF_PROCESSORS=1
    fi  

    cmake -G "MSYS Makefiles" $WORKSPACE || die 1 "Failed to do the first cmake - Halting"

    make dist || die 2 "Failed to build dist - Halting"

    cmake -G "MSYS Makefiles" -Denable_java=ON -Denable_model-checking=OFF -Denable_lua=OFF -Denable_compile_optimizations=ON  -Denable_smpi=ON -Denable_smpi_MPICH3_testsuite=ON -Denable_compile_warnings=OFF . \
    || die 5 "Failed to perform the Cmake for $build_mode - Halting"

    make -j$NUMBER_OF_PROCESSORS || die 5 "Build failure - Halting"

    make nsis || die 6 "Failure while generating the Windows executable - Halting"

else
    # Linux:
    cpuinfo_file="/proc/cpuinfo"
    NUMBER_OF_PROCESSORS=$(lscpu -p 2>/dev/null | grep -c '^[^#]') || \
    NUMBER_OF_PROCESSORS=$(grep -c "^processor[[:space:]]*:" ${cpuinfo_file} 2>/dev/null)
    [ "0$NUMBER_OF_PROCESSORS" -gt 0 ] || NUMBER_OF_PROCESSORS=1

    cmake $WORKSPACE || die 1 "Failed to do the first cmake - Halting"

    rm Simgrid*.tar.gz
    make dist || die 2 "Failed to build dist - Halting"

    tar xzf `cat VERSION`.tar.gz || die 3 "Failed to extract the generated tgz - Halting"

    cd `cat VERSION` || die 4 "Path `cat VERSION` cannot be found - Halting"

    if [ "$build_mode" = "Debug" ]
    then
    cmake -Denable_coverage=OFF -Denable_java=ON -Denable_model-checking=OFF -Denable_lua=ON -Denable_compile_optimizations=ON  -Denable_smpi=ON -Denable_smpi_MPICH3_testsuite=ON -Denable_compile_warnings=ON .
    fi

    if [ "$build_mode" = "ModelChecker" ]
    then
    cmake -Denable_coverage=OFF -Denable_java=ON -Denable_smpi=ON -Denable_model-checking=ON -Denable_lua=ON -Denable_compile_optimizations=OFF -Denable_compile_warnings=ON .
    fi

    if [ "$build_mode" = "DynamicAnalysis" ]
    then
    cmake -Denable_lua=OFF -Denable_java=ON -Denable_tracing=ON -Denable_smpi=ON -Denable_compile_optimizations=OFF -Denable_compile_warnings=ON -Denable_lib_static=OFF -Denable_model-checking=OFF -Denable_latency_bound_tracking=OFF -Denable_gtnets=OFF -Denable_jedule=OFF -Denable_mallocators=OFF -Denable_memcheck=ON .
    fi

    [ $? -eq 0 ] || die 5 "Failed to perform the Cmake for $build_mode - Halting"

    make -j$NUMBER_OF_PROCESSORS || die 6 "Build failure - Halting"
fi

echo "running tests with $NUMBER_OF_PROCESSORS processors"

ctest --output-on-failure -T test --no-compress-output  --timeout 100 -j$NUMBER_OF_PROCESSORS || true
if [ -f Testing/TAG ] ; then
   xsltproc $WORKSPACE/buildtools/jenkins/ctest2junit.xsl -o "$WORKSPACE/CTestResults.xml" Testing/`head -n 1 < Testing/TAG`/Test.xml
fi

ctest -D ContinuousStart
ctest -D ContinuousConfigure
ctest -D ContinuousBuild
ctest -D ContinuousTest
ctest -D ContinuousSubmit

rm -rf `cat VERSION`
