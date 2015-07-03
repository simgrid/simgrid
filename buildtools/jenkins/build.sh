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

# Get an ON/OFF string from a command:
onoff() {
  if "$@" > /dev/null ; then
    echo ON
  else
    echo OFF
  fi
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

case "$build_mode" in
  "Debug")
  ;;

  "ModelChecker")
  ;;

  "DynamicAnalysis")
  ;;

  *)
  die 1 "Unknown build_mode $build_mode"
  ;;
esac

cmake -Denable_debug=ON -Denable_documentation=OFF -Denable_coverage=OFF \
  -Denable_model-checking=$(onoff test "$build_mode" = "ModelChecker") \
  -Denable_compile_optimization=$(onoff test "$build_mode" = "Debug") \
  -Denable_smpi_MPICH3_testsuite=$(onoff test "$build_mode" != "DynamicAnalysis") \
  -Denable_lua=$(onoff test "$build_mode" != "DynamicAnalysis") \
  -Denable_mallocators=$(onoff test "$build_mode" != "DynamicAnalysis") \
  -Denable_memcheck=$(onoff test "$build_mode" = "DynamicAnalysis") \
  -Denable_compile_warnings=ON -Denable_smpi=ON -Denable_lib_static=OFF \
  -Denable_latency_bound_tracking=OFF -Denable_gtnets=OFF -Denable_jedule=OFF \
  -Denable_tracing=ON -Denable_java=ON
make

cd $WORKSPACE/build
cd `cat VERSION`

TRES=0

ctest -T test --no-compress-output || true
if [ -f Testing/TAG ] ; then
   xsltproc $WORKSPACE/buildtools/jenkins/ctest2junit.xsl Testing/`head -n 1 < Testing/TAG`/Test.xml > CTestResults.xml
   mv CTestResults.xml $WORKSPACE
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
