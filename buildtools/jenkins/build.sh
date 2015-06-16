#!/bin/sh

set -e

build_mode="$1"

# usage: die status message...
die () {
  local status=${1:-1}
  shift
  [ $# -gt 0 ] || set -- "Error - Halting"
  echo "$@" >&2
  exit $status
}

ulimit -c 0

if [ -d $WORKSPACE/build ]
then
  rm -rf $WORKSPACE/build
fi
mkdir $WORKSPACE/build
cd $WORKSPACE/build

cmake -Denable_documentation=OFF $WORKSPACE
make dist
tar xzf `cat VERSION`.tar.gz
cd `cat VERSION`

if [ "$build_mode" = "Debug" ]
then
cmake -Denable_documentation=OFF -Denable_coverage=OFF -Denable_java=ON -Denable_smpi_MPICH3_testsuite=ON -Denable_model-checking=OFF -Denable_lua=ON -Denable_compile_optimizations=ON -Denable_compile_warnings=ON .
fi

if [ "$build_mode" = "ModelChecker" ]
then
cmake -Denable_documentation=OFF -Denable_coverage=OFF -Denable_java=ON -Denable_smpi_MPICH3_testsuite=ON -Denable_model-checking=ON -Denable_lua=ON -Denable_compile_optimizations=OFF -Denable_compile_warnings=ON .
fi

if [ "$build_mode" = "DynamicAnalysis" ]
then
cmake -Denable_documentation=OFF -Denable_lua=OFF -Denable_java=ON -Denable_tracing=ON -Denable_smpi=ON -Denable_compile_optimizations=OFF -Denable_compile_warnings=ON -Denable_lib_static=OFF -Denable_model-checking=OFF -Denable_latency_bound_tracking=OFF -Denable_gtnets=OFF -Denable_jedule=OFF -Denable_mallocators=OFF -Denable_memcheck=ON -Denable_smpi_MPICH3_testsuite=OFF .
fi

make

cd $WORKSPACE/build
cd `cat VERSION`

TRES=0

ctest -T test --no-compress-output || true
if [ -f Testing/TAG ] ; then
   xsltproc $WORKSPACE/buildtools/jenkins/ctest2junit.xsl Testing/`head -n 1 < Testing/TAG`/Test.xml > CTestResults.xml
   mv CTestResults.xml $WORKSPACE
fi

if [ "$build_mode" = "Debug" ]
then
cmake -Denable_documentation=OFF -Denable_coverage=OFF -Denable_java=ON -Denable_smpi_MPICH3_testsuite=ON -Denable_model-checking=OFF -Denable_lua=ON -Denable_compile_optimizations=ON -Denable_compile_warnings=ON .
fi

if [ "$build_mode" = "ModelChecker" ]
then
cmake -Denable_documentation=OFF -Denable_coverage=OFF -Denable_java=ON -Denable_smpi_MPICH3_testsuite=ON -Denable_model-checking=ON -Denable_lua=ON -Denable_compile_optimizations=OFF -Denable_compile_warnings=ON .
fi

if [ "$build_mode" = "DynamicAnalysis" ]
then
  ctest -D ContinuousStart
  ctest -D ContinuousConfigure
  ctest -D ContinuousBuild
  ctest -D ContinuousMemCheck
  ctest -D ContinuousSubmit
fi

ctest -D ContinuousStart
ctest -D ContinuousConfigure
ctest -D ContinuousBuild
ctest -D ContinuousTest
ctest -D ContinuousSubmit

rm -rf `cat VERSION`
