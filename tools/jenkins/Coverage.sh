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

ctest -D ExperimentalStart || true

cmake -Denable_documentation=OFF -Denable_lua=OFF -Denable_java=ON \
      -Denable_compile_optimizations=OFF -Denable_compile_warnings=ON \
      -Denable_jedule=OFF -Denable_mallocators=ON \
      -Denable_smpi=ON -Denable_smpi_MPICH3_testsuite=OFF -Denable_model-checking=ON \
      -Denable_memcheck=OFF -Denable_memcheck_xml=OFF -Denable_smpi_ISP_testsuite=OFF -Denable_coverage=ON $WORKSPACE

make -j$NUMPROC
JACOCO_PATH="/usr/local/share/jacoco"
export JAVA_TOOL_OPTIONS="-javaagent:${JACOCO_PATH}/lib/jacocoagent.jar"

ctest -D ExperimentalTest -j$NUMPROC || true
ctest -D ExperimentalCoverage || true

unset JAVA_TOOL_OPTIONS
i=0
if [ -f Testing/TAG ] ; then
  for example in app/bittorrent app/centralizedmutex app/masterworker app/pingpong app/tokenring async/yield async/waitall async/dsend cloud/migration cloud/masterworker dht/chord dht/kademlia energy/consumption energy/pstate energy/vm io/file io/storage process/kill process/migration process/startkilltime process/suspend task/priority trace/pingpong
  do
    #convert jacoco reports in xml ones
    ant -f $WORKSPACE/tools/jenkins/jacoco.xml -Dsrcdir=$WORKSPACE/examples/java/${example} -Dbuilddir=$WORKSPACE/build/examples/java/${example} -Djacocodir=${JACOCO_PATH}/lib
    #convert jacoco xml reports in cobertura xml reports
    cover2cover.py $WORKSPACE/build/examples/java/${example}/report.xml $WORKSPACE/examples/java/ > $WORKSPACE/java_coverage_${i}.xml
  i=$(($i + 1))
  done
   #convert all gcov reports to xml cobertura reports
   gcovr -r .. --xml-pretty -e teshsuite.* -u -o $WORKSPACE/xml_coverage.xml
   xsltproc $WORKSPACE/tools/jenkins/ctest2junit.xsl Testing/`head -n 1 < Testing/TAG`/Test.xml > CTestResults_memcheck.xml
   mv CTestResults_memcheck.xml $WORKSPACE
fi
