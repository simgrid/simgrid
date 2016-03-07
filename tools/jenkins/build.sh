#!/bin/sh

# This script is used by various build projects on Jenkins

# See https://ci.inria.fr/simgrid/job/SimGrid-Multi/configure
# See https://ci.inria.fr/simgrid/job/Simgrid-Windows/configure

set -e

# ensure that the locales are set, so that perl keeps its nerves
export LC_ALL=C

echo "XXXX Cleanup previous attempts. Remaining content of /tmp:"
rm -rf /tmp/simgrid-java*
rm -rf /tmp/jvm-* 
find /builds/workspace/SimGrid-Multi/ -name "hs_err_pid*.log" | xargs rm -f
ls /tmp
df -h
echo "XXXX Let's go"

# Help older cmakes
if [ -e /usr/lib/jvm/java-7-openjdk-amd64 ] ; 
then
  export JAVA_HOME=/usr/lib/jvm/java-7-openjdk-amd64
fi

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

# Check that we have what we need, or die quickly.
# The paths are not the same on all platforms, unfortunately.
#test -e /bin/tar  || die 1 "I need tar to compile. Please fix your slave."
#test -e /bin/gzip || die 1 "I need gzip to compile. Please fix your slave."
#test -e /usr/include/libunwind.h || die 1 "I need libunwind to compile. Please fix your slave."
#test -e /usr/include/valgrind/valgrind.h || die 1 "I need valgrind to compile. Please fix your slave."

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

echo "XX"
echo "XX Get out of the tree"
echo "XX"
if [ -d $WORKSPACE/build ]
then
  rm -rf $WORKSPACE/build
fi
mkdir $WORKSPACE/build
cd $WORKSPACE/build

if test "$(uname -o)" != "Msys"; then
  echo "XX"
  echo "XX Build the archive out of the tree"
  echo "XX   pwd: `pwd`"
  echo "XX"

  cmake -G"$GENERATOR" -Denable_documentation=OFF $WORKSPACE
  make dist -j$NUMBER_OF_PROCESSORS

  echo "XX"
  echo "XX Open the resulting archive"
  echo "XX"
  tar xzf `cat VERSION`.tar.gz
  cd `cat VERSION`
  SRCFOLDER="."
else
#for windows we don't make dist, but we still want to build out of source
  SRCFOLDER=$WORKSPACE
fi

echo "XX"
echo "XX Configure and build SimGrid"
echo "XX   pwd: `pwd`"
echo "XX"
cmake -G"$GENERATOR"\
  -Denable_debug=ON -Denable_documentation=OFF -Denable_coverage=OFF \
  -Denable_model-checking=$(onoff test "$build_mode" = "ModelChecker") \
  -Denable_smpi_ISP_testsuite=$(onoff test "$build_mode" = "ModelChecker") \
  -Denable_compile_optimizations=$(onoff test "$build_mode" = "Debug") \
  -Denable_smpi_MPICH3_testsuite=$(onoff test "$build_mode" != "DynamicAnalysis") \
  -Denable_mallocators=$(onoff test "$build_mode" != "DynamicAnalysis") \
  -Denable_memcheck=$(onoff test "$build_mode" = "DynamicAnalysis") \
  -Denable_compile_warnings=$(onoff test "$GENERATOR" != "MSYS Makefiles") -Denable_smpi=ON \
  -Denable_jedule=OFF -Denable_java=ON -Denable_lua=OFF $SRCFOLDER
#  -Denable_lua=$(onoff test "$build_mode" != "DynamicAnalysis") \

make -j$NUMBER_OF_PROCESSORS VERBOSE=1

if test "$(uname -o)" != "Msys"; then
  cd $WORKSPACE/build
  cd `cat VERSION`
fi

TRES=0

echo "XX"
echo "XX Run the tests"
echo "XX   pwd: `pwd`"
echo "XX"

ctest -T test --output-on-failure --no-compress-output || true
if [ -f Testing/TAG ] ; then
   xsltproc $WORKSPACE/tools/jenkins/ctest2junit.xsl Testing/`head -n 1 < Testing/TAG`/Test.xml > CTestResults.xml
   mv CTestResults.xml $WORKSPACE
fi

echo "XX"
echo "XX Done. Return the results to cmake"
echo "XX"
