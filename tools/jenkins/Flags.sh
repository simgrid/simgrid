#!/usr/bin/env sh

set -e

echo "Starting Flags.sh $*"

die() {
    echo "$@"
    exit 1
}

# Get an ON/OFF string from a command:
onoff() {
  if "$@" > /dev/null ; then
    echo ON
  else
    echo OFF
  fi
}

[ $# -eq 3 ] || die "Needs 3 arguments : MC SMPI DEBUG"

### Cleanup previous runs

[ -n "$WORKSPACE" ] || die "No WORKSPACE"
[ -d "$WORKSPACE" ] || die "WORKSPACE ($WORKSPACE) does not exist"

do_cleanup() {
  for d
  do
    if [ -d "$d" ]
    then
      rm -rf "$d" || die "Could not remove $d"
    fi
    mkdir "$d" || die "Could not create $d"
  done
}

do_cleanup "$WORKSPACE/build"

NUMPROC="$(nproc)" || NUMPROC=1

cd "$WORKSPACE"/build

#we can't just receive ON or OFF as values as display is bad in the resulting jenkins matrix

if [ "$1" = "MC" ]
then
  buildmc="ON"
else
  buildmc="OFF"
fi

if [ "$2" = "SMPI" ]
then
  buildsmpi="ON"
else
  buildsmpi="OFF"
fi

if [ "$3" = "DEBUG" ]
then
  builddebug="ON"
else
  builddebug="OFF"
fi

echo "Step ${STEP}/${NSTEPS} - Building with debug=${builddebug}, SMPI=${buildsmpi}, MC=${buildmc}"

if [ "${builddebug}/${buildsmpi}/${buildmc}" = "ON/ON/ON" ]; then
    # ${buildmc}=ON because "why not", and especially because it doesn't
    # compile with -D_GLIBCXX_DEBUG and -Denable_ns3=ON together
    export CXXFLAGS=-D_GLIBCXX_DEBUG
    runtests="ON"
else
    runtests="OFF"
fi

GENERATOR="Unix Makefiles"
BUILDER=make
VERBOSE_BUILD="VERBOSE=1"
if which ninja 2>/dev/null >/dev/null ; then
  GENERATOR=Ninja
  BUILDER=ninja
  VERBOSE_BUILD="-v"
fi

cmake -G"$GENERATOR" -Denable_documentation=OFF \
      -Denable_compile_optimizations=${runtests} -Denable_compile_warnings=ON \
      -Denable_mallocators=ON -Denable_debug=${builddebug} \
      -Denable_smpi=${buildsmpi} -Denable_testsuite_smpi_MPICH3=${buildsmpi} -Denable_model-checking=${buildmc} \
      -Denable_memcheck=OFF -Denable_memcheck_xml=OFF -Denable_testsuite_smpi_MBI=OFF -Denable_testsuite_McMini=OFF \
      -Denable_ns3=$(onoff test "$buildmc" != "ON") -DNS3_HINT=/builds/ns-3-dev/build/ \
      -Denable_coverage=OFF -DLTO_EXTRA_FLAG="auto" -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
      "$WORKSPACE"

${BUILDER} -j$NUMPROC tests ${VERBOSE_BUILD}

if [ "$runtests" = "ON" ]; then
    # exclude tests known to fail with _GLIBCXX_DEBUG
    ctest -j$NUMPROC -E '^[ps]thread-|mc-bugged1-liveness' --output-on-failure
fi

cd ..
rm -rf build
