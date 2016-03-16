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
  find $WORKSPACE -name "memcheck_test_*.memcheck" -exec rm {} \;
}

### Check the node installation

for pkg in xsltproc gcovr
do
   if command -v $pkg
   then 
      echo "$pkg is installed. Good."
   else 
      die "please install $pkg before proceeding" 
   fi
done

### Cleanup previous runs

! [ -z "$WORKSPACE" ] || die "No WORKSPACE"
[ -d "$WORKSPACE" ] || die "WORKSPACE ($WORKSPACE) does not exist"

do_cleanup

for d in "$WORKSPACE/build"
do
  mkdir "$d" || die "Could not create $d"
done

cd $WORKSPACE/build

ctest -D ExperimentalStart || true

cmake -Denable_documentation=OFF -Denable_lua=ON -Denable_java=ON \
      -Denable_compile_optimizations=OFF -Denable_compile_warnings=ON \
      -Denable_jedule=ON -Denable_mallocators=ON \
      -Denable_smpi=ON -Denable_smpi_MPICH3_testsuite=ON -Denable_model-checking=ON \
      -Denable_memcheck=OFF -Denable_memcheck_xml=OFF -Denable_smpi_ISP_testsuite=ON -Denable_coverage=ON $WORKSPACE

ctest -D ExperimentalBuild -V
ctest -D ExperimentalTest -E liveness || true
ctest -D ExperimentalCoverage || true

if [ -f Testing/TAG ] ; then
   gcovr -r .. --xml-pretty -e teshsuite.* -u -o $WORKSPACE/xml_coverage.xml
   xsltproc $WORKSPACE/tools/jenkins/ctest2junit.xsl Testing/`head -n 1 < Testing/TAG`/Test.xml > CTestResults_memcheck.xml
   mv CTestResults_memcheck.xml $WORKSPACE
fi
