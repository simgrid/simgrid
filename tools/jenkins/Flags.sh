#!/bin/sh

set -e

die() {
    echo "$@"
    exit 1
}

do_cleanup() {
  for d in "$WORKSPACE/build"
  do
    if [ -d "$d" ]
    then
      rm -rf "$d" || die "Could not remote $d"
    fi
  done
}

# Get an ON/OFF string from a command:
onoff() {
  if "$@" > /dev/null ; then
    echo ON
  else
    echo OFF
  fi
}

### Cleanup previous runs

! [ -z "$WORKSPACE" ] || die "No WORKSPACE"
[ -d "$WORKSPACE" ] || die "WORKSPACE ($WORKSPACE) does not exist"

do_cleanup

for d in "$WORKSPACE/build"
do
  mkdir "$d" || die "Could not create $d"
done

NUMPROC="$(nproc)" || NUMPROC=1

cd $WORKSPACE/build

NSTEPS=16
STEP=1

for buildjava in ON OFF
do
  for builddebug in ON OFF
  do
    for buildsmpi in ON OFF
    do
      for buildmc in ON OFF
      do
        echo "Step ${STEP}/${NSTEPS} - Building with java=${buildjava}, debug=${builddebug}, SMPI=${buildsmpi}, MC=${buildmc}"
        echo "Step ${STEP}/${NSTEPS} - Building with java=${buildjava}, debug=${builddebug}, SMPI=${buildsmpi}, MC=${buildmc}" > $WORKSPACE/buildstep
        cmake -Denable_documentation=OFF -Denable_lua=ON -Denable_java=${buildjava} \
              -Denable_compile_optimizations=OFF -Denable_compile_warnings=ON \
              -Denable_jedule=ON -Denable_mallocators=ON -Denable_debug=${builddebug} \
              -Denable_smpi=${buildsmpi} -Denable_smpi_MPICH3_testsuite=${buildsmpi} -Denable_model-checking=${buildmc} \
              -Denable_memcheck=OFF -Denable_memcheck_xml=OFF -Denable_smpi_ISP_testsuite=OFF \
              -Denable_ns3=$(onoff test "$buildmc" != "ON") -Denable_coverage=OFF $WORKSPACE
        make -j$NUMPROC
        make clean
        STEP=$((${STEP}+1))
      done
    done
  done
done 

echo "All builds finished cleanly" > $WORKSPACE/buildstep

