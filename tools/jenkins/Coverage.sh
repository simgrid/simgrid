#!/bin/sh

set -e

BUILDFOLDER=$WORKSPACE/build

die() {
    echo "$@"
    exit 1
}

do_cleanup() {
  for d in "$BUILDFOLDER"
  do
    if [ -d "$d" ]
    then
      rm -rf "$d" || die "Could not remote $d"
    fi
  done
}

### Check the node installation

for pkg in xsltproc gcovr ant cover2cover.py
do
   if command -v $pkg
   then 
      echo "$pkg is installed. Good."
   else 
      die "please install $pkg before proceeding" 
   fi
done

### Cleanup previous runs

! [ -z "$WORKSPACE" ] || die "No WORKSPACE"
[ -d "$WORKSPACE" ] || die "WORKSPACE ($WORKSPACE) does not exist"

do_cleanup

for d in "$BUILDFOLDER"
do
  mkdir "$d" || die "Could not create $d"
done

NUMPROC="$(nproc)" || NUMPROC=1


cd $BUILDFOLDER

ctest -D ExperimentalStart || true

cmake -Denable_documentation=OFF -Denable_lua=ON -Denable_java=ON \
      -Denable_compile_optimizations=OFF -Denable_compile_warnings=ON \
      -Denable_jedule=ON -Denable_mallocators=ON \
      -Denable_smpi=ON -Denable_smpi_MPICH3_testsuite=ON -Denable_model-checking=ON \
      -Denable_memcheck=OFF -Denable_memcheck_xml=OFF -Denable_smpi_ISP_testsuite=ON -Denable_coverage=ON $WORKSPACE

make -j$NUMPROC
JACOCO_PATH="/usr/local/share/jacoco"
export JAVA_TOOL_OPTIONS="-javaagent:${JACOCO_PATH}/lib/jacocoagent.jar"

ctest --no-compress-output -D ExperimentalTest -j$NUMPROC || true
ctest -D ExperimentalCoverage || true

unset JAVA_TOOL_OPTIONS
if [ -f Testing/TAG ] ; then

  files=$( find . -name "jacoco.exec" )
  i=0
  for file in $files
  do
    sourcepath=$( dirname $file )
    #convert jacoco reports in xml ones
    ant -f $WORKSPACE/tools/jenkins/jacoco.xml -Dexamplesrcdir=$WORKSPACE -Dbuilddir=$BUILDFOLDER/${sourcepath} -Djarfile=$BUILDFOLDER/simgrid.jar -Djacocodir=${JACOCO_PATH}/lib
    #convert jacoco xml reports in cobertura xml reports
    cover2cover.py $BUILDFOLDER/${sourcepath}/report.xml .. ../src/bindings/java src/bindings/java > $WORKSPACE/java_coverage_${i}.xml
    i=$((i + 1))
  done

   #convert all gcov reports to xml cobertura reports
   gcovr -r .. --xml-pretty -e teshsuite.* -u -o $WORKSPACE/xml_coverage.xml
   xsltproc $WORKSPACE/tools/jenkins/ctest2junit.xsl Testing/$( head -n 1 < Testing/TAG )/Test.xml > CTestResults_memcheck.xml
   mv CTestResults_memcheck.xml $WORKSPACE
fi
