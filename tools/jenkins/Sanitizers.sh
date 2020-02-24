#!/usr/bin/env sh

set -e

die() {
    echo "$@"
    exit 1
}

if [ -z "$1" ]
  then
    echo "No Sanitizer type selected - run Address"
    SANITIZER="address"
else
    SANITIZER=$1
fi


if [ "${SANITIZER}" = "address" ]
then
    export ASAN_OPTIONS="suppressions=$WORKSPACE/tools/address_sanitizer.supp"
    SANITIZER_OPTIONS="-Denable_address_sanitizer=ON -Denable_undefined_sanitizer=OFF -Denable_thread_sanitizer=OFF"
elif [ "${SANITIZER}" = "thread" ]
then
    export TSAN_OPTIONS="memory_limit_mb=1500 suppressions=$WORKSPACE/tools/thread_sanitizer.supp"
    SANITIZER_OPTIONS="-Denable_address_sanitizer=OFF -Denable_undefined_sanitizer=OFF -Denable_thread_sanitizer=ON"
elif [ "${SANITIZER}" = "undefined" ]
then
    export UBSAN_OPTIONS="print_stacktrace=1"
    SANITIZER_OPTIONS="-Denable_address_sanitizer=OFF -Denable_undefined_sanitizer=ON -Denable_thread_sanitizer=OFF"
else
    die "Unknown Sanitizer type selected  ${SANITIZER} - Exiting"
fi



### Check the node installation

pkg_check() {
  for pkg
  do
    if command -v $pkg
    then
       echo "$pkg is installed. Good."
    else
       die "please install $pkg before proceeding"
    fi
  done
}

pkg_check xsltproc

### Cleanup previous runs

! [ -z "$WORKSPACE" ] || die "No WORKSPACE"
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

cd $WORKSPACE/build

ctest -D ExperimentalStart || true

cmake -Denable_documentation=OFF -Denable_lua=ON -Denable_java=OFF \
      -Denable_compile_optimizations=ON -Denable_compile_warnings=ON \
      -Denable_jedule=ON -Denable_mallocators=OFF \
      -Denable_smpi=ON -Denable_smpi_MPICH3_testsuite=ON -Denable_model-checking=OFF \
      -Denable_memcheck=OFF -Denable_memcheck_xml=OFF -Denable_smpi_ISP_testsuite=ON -Denable_coverage=OFF\
      -Denable_fortran=OFF -Denable_python=OFF -DLTO_EXTRA_FLAG="auto" ${SANITIZER_OPTIONS} $WORKSPACE

make -j$NUMPROC tests
ctest --no-compress-output -D ExperimentalTest || true

if [ -f Testing/TAG ] ; then
   xsltproc $WORKSPACE/tools/jenkins/ctest2junit.xsl Testing/$(head -n 1 < Testing/TAG)/Test.xml > CTestResults_${SANITIZER}.xml
   mv CTestResults_${SANITIZER}.xml $WORKSPACE
fi

make clean
