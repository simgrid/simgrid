#!/bin/sh

set -e

die() {
    echo "$@"
    exit 1
}

do_cleanup() {
  for d in "$WORKSPACE/build" "$WORKSPACE/memcheck"
  do
    if [ -d "$d" ]
    then
      rm -rf "$d" || die "Could not remote $d"
    fi
  done
  find $WORKSPACE -name "memcheck_test_*.memcheck" -exec rm {} \;
}

! [ -z "$WORKSPACE" ] || die "No WORKSPACE"
[ -d "$WORKSPACE" ] || die "WORKSPACE ($WORKSPACE) does not exist"

do_cleanup

for d in "$WORKSPACE/build" "$WORKSPACE/memcheck"
do
  mkdir "$d" || die "Could not create $d"
done

cd $WORKSPACE/build

cmake -Denable_documentation=OFF -Denable_lua=OFF -Denable_tracing=ON \
      -Denable_compile_optimizations=OFF -Denable_compile_warnings=ON -Denable_lib_static=OFF \
      -Denable_latency_bound_tracking=OFF -Denable_jedule=OFF -Denable_mallocators=OFF \
      -Denable_smpi=ON -Denable_smpi_MPICH3_testsuite=OFF -Denable_model-checking=OFF \
      -Denable_memcheck_xml=ON $WORKSPACE
make

ctest -D ExperimentalStart || true
ctest -D ExperimentalConfigure || true
ctest -D ExperimentalBuild || true
ctest -D ExperimentalMemCheck || true

cd $WORKSPACE/build
if [ -f Testing/TAG ] ; then
   find . -iname "*.memcheck" -exec mv {} $WORKSPACE/memcheck \;
   mv Testing/`head -n 1 < Testing/TAG`/DynamicAnalysis.xml  $WORKSPACE
fi

make clean

cmake -Denable_documentation=OFF -Denable_lua=OFF -Denable_tracing=ON \
      -Denable_compile_optimizations=OFF -Denable_compile_warnings=ON -Denable_lib_static=OFF \
      -Denable_latency_bound_tracking=OFF -Denable_jedule=OFF -Denable_mallocators=OFF \
      -Denable_smpi=ON -Denable_smpi_MPICH3_testsuite=OFF -Denable_model-checking=OFF  \
      -Denable_memcheck=OFF -Denable_memcheck_xml=OFF -Denable_coverage=ON $WORKSPACE

make
ctest -D ExperimentalStart || true
ctest -D ExperimentalConfigure || true
ctest -D ExperimentalBuild || true
ctest -D ExperimentalTest || true
ctest -D ExperimentalCoverage || true

if [ -f Testing/TAG ] ; then
   /usr/local/gcovr-3.1/scripts/gcovr -r .. --xml-pretty -o $WORKSPACE/xml_coverage.xml
   xsltproc $WORKSPACE/buildtools/jenkins/ctest2junit.xsl Testing/`head -n 1 < Testing/TAG`/Test.xml > CTestResults_memcheck.xml
   mv CTestResults_memcheck.xml $WORKSPACE
fi

do_cleanup
