#!/bin/sh

set -e

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

build_mode="$1"
echo "Build mode $build_mode on $(uname -np)" >&2
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

if test "$(uname -o)" = "Msys"; then
  if [ -z "$NUMBER_OF_PROCESSORS" ]; then
    NUMBER_OF_PROCESSORS=1
  fi
  GENERATOR="MSYS Makefiles"
else
  NUMBER_OF_PROCESSORS="$(nproc)" || NUMBER_OF_PROCESSORS=1
  GENERATOR="Unix Makefiles"
fi

ulimit -c 0 || true

if [ -d $WORKSPACE/build ]
then
  rm -rf $WORKSPACE/build
fi
mkdir $WORKSPACE/build
cd $WORKSPACE/build

cmake -G"$GENERATOR" -Denable_documentation=OFF $WORKSPACE
make dist -j$NUMBER_OF_PROCESSORS
tar xzf `cat VERSION`.tar.gz
cd `cat VERSION`

cmake -G"$GENERATOR"\
  -Denable_debug=ON -Denable_documentation=OFF -Denable_coverage=OFF \
  -Denable_model-checking=$(onoff test "$build_mode" = "ModelChecker") \
  -Denable_compile_optimizations=$(onoff test "$build_mode" = "Debug") \
  -Denable_smpi_MPICH3_testsuite=$(onoff test "$build_mode" != "DynamicAnalysis") \
  -Denable_lua=$(onoff test "$build_mode" != "DynamicAnalysis") \
  -Denable_mallocators=$(onoff test "$build_mode" != "DynamicAnalysis") \
  -Denable_memcheck=$(onoff test "$build_mode" = "DynamicAnalysis") \
  -Denable_compile_warnings=ON -Denable_smpi=ON -Denable_lib_static=OFF \
  -Denable_latency_bound_tracking=OFF -Denable_gtnets=OFF -Denable_jedule=OFF \
  -Denable_tracing=ON -Denable_java=ON
make -j$NUMBER_OF_PROCESSORS VERBOSE=1

cd $WORKSPACE/build
cd `cat VERSION`

TRES=0

ctest -T test --output-on-failure --no-compress-output || true
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
