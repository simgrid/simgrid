#!/usr/bin/env sh

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
#test -e /usr/include/valgrind/valgrind.h || die 1 "I need valgrind to compile. Please fix your slave."

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

NUMBER_OF_PROCESSORS="$(nproc)" || NUMBER_OF_PROCESSORS=1
GENERATOR="Unix Makefiles"

ulimit -c 0 || true

echo "XX"
echo "XX Get out of the tree"
echo "XX"
if [ -d $WORKSPACE/build ]
then
  # Windows cannot remove the directory if it's still used by the previous build
  rm -rf $WORKSPACE/build || sleep 10 && rm -rf $WORKSPACE/build || sleep 10 && rm -rf $WORKSPACE/build
fi
mkdir $WORKSPACE/build
cd $WORKSPACE/build

have_NS3="no"
if dpkg -l libns3-dev 2>&1|grep -q "ii  libns3-dev" ; then
  have_NS3="yes"
fi
echo "XX have_NS3: ${have_NS3}"

# This is for Windows:
PATH="$WORKSPACE/build/lib:$PATH"

echo "XX"
echo "XX Build the archive out of the tree"
echo "XX   pwd: "$(pwd)
echo "XX"

cmake -G"$GENERATOR" -Denable_documentation=OFF $WORKSPACE
make dist -j$NUMBER_OF_PROCESSORS
SIMGRID_VERSION=$(cat VERSION)

echo "XX"
echo "XX Open the resulting archive"
echo "XX"
gunzip ${SIMGRID_VERSION}.tar.gz
tar xf ${SIMGRID_VERSION}.tar
mkdir ${WORKSPACE}/build/${SIMGRID_VERSION}/build
cd ${WORKSPACE}/build/${SIMGRID_VERSION}/build
SRCFOLDER="${WORKSPACE}/build/${SIMGRID_VERSION}"

echo "XX"
echo "XX Configure and build SimGrid"
echo "XX   pwd: "$(pwd)
echo "XX"
set -x
if [ "$build_mode" = "ModelChecker" ] ; then
   INSTALL="-DCMAKE_INSTALL_PREFIX=/builds/mc_simgrid_install"
elif [ "$build_mode" = "Debug" ] ; then
   INSTALL="-DCMAKE_INSTALL_PREFIX=/builds/simgrid_install"
fi

if cmake --version | grep -q 3\.11 ; then
  # -DCMAKE_DISABLE_SOURCE_CHANGES=ON is broken with java on CMake 3.11
  # https://gitlab.kitware.com/cmake/cmake/issues/17933
  MAY_DISABLE_SOURCE_CHANGE=""
else 
  MAY_DISABLE_SOURCE_CHANGE="-DCMAKE_DISABLE_SOURCE_CHANGES=ON"
fi

cmake -G"$GENERATOR" $INSTALL \
  -Denable_debug=ON -Denable_documentation=OFF -Denable_coverage=OFF \
  -Denable_model-checking=$(onoff test "$build_mode" = "ModelChecker") \
  -Denable_smpi_ISP_testsuite=$(onoff test "$build_mode" = "ModelChecker") \
  -Denable_compile_optimizations=$(onoff test "$build_mode" != "DynamicAnalysis") \
  -Denable_smpi_MPICH3_testsuite=$(onoff test "$build_mode" = "Debug") \
  -Denable_mallocators=$(onoff test "$build_mode" != "DynamicAnalysis") \
  -Denable_memcheck=$(onoff test "$build_mode" = "DynamicAnalysis") \
  -Denable_compile_warnings=$(onoff test "$GENERATOR" != "MSYS Makefiles") -Denable_smpi=ON \
  -Denable_ns3=$(onoff test "$have_NS3" = "yes" -a "$build_mode" = "Debug") \
  -Denable_jedule=OFF -Denable_java=ON -Denable_lua=OFF ${MAY_DISABLE_SOURCE_CHANGE} \
  $SRCFOLDER
#  -Denable_lua=$(onoff test "$build_mode" != "DynamicAnalysis") \
set +x

make -j$NUMBER_OF_PROCESSORS VERBOSE=1

echo "XX"
echo "XX Run the tests"
echo "XX   pwd: "$(pwd)
echo "XX"

ctest -T test --output-on-failure --no-compress-output || true
if [ -f Testing/TAG ] ; then
   xsltproc $WORKSPACE/tools/jenkins/ctest2junit.xsl Testing/$( head -n 1 < Testing/TAG )/Test.xml > CTestResults.xml
   mv CTestResults.xml $WORKSPACE
fi

if test "$(uname)" != "Msys" && test "${build_mode}" = "Debug" -o "${build_mode}" = "ModelChecker" ; then
  echo "XX"
  echo "XX Test done. Install everything since it's a regular build, not on a Windows."
  echo "XX"

  test "${build_mode}" = "Debug"        && rm -rf /builds/simgrid_install
  test "${build_mode}" = "ModelChecker" && rm -rf /builds/mc_simgrid_install

  make install
fi

echo "XX"
echo "XX Done. Return the results to cmake"
echo "XX"
