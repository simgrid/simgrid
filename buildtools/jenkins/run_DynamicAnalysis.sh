#!/bin/sh
if [ -d $WORKSPACE/build ]
then
  rm -rf $WORKSPACE/build
fi
if [ -d $WORKSPACE/install ]
then
  rm -rf $WORKSPACE/install
fi
mkdir $WORKSPACE/build
mkdir $WORKSPACE/install

if [ -d $WORKSPACE/memcheck ]
then
  rm -rf $WORKSPACE/memcheck
fi
mkdir $WORKSPACE/memcheck

cd $WORKSPACE/build

cmake -Denable_lua=OFF -Denable_tracing=ON -Denable_smpi=ON  -Denable_smpi_MPICH3_testsuite=OFF -Denable_compile_optimizations=OFF -Denable_compile_warnings=ON -Denable_lib_static=OFF -Denable_model-checking=OFF -Denable_latency_bound_tracking=OFF -Denable_gtnets=OFF -Denable_jedule=OFF -Denable_mallocators=OFF -Denable_memcheck_xml=ON $WORKSPACE
make

ctest -D ExperimentalStart
ctest -D ExperimentalConfigure
ctest -D ExperimentalBuild
ctest -D ExperimentalMemCheck

cd $WORKSPACE/build
if [ -f Testing/TAG ] ; then
   find . -iname "*.memcheck" -exec mv {} $WORKSPACE/memcheck \;
   mv Testing/`head -n 1 < Testing/TAG`/DynamicAnalysis.xml  $WORKSPACE
fi

make clean

cmake -Denable_lua=OFF -Denable_tracing=ON -Denable_smpi=ON -Denable_smpi_MPICH3_testsuite=ON -Denable_compile_optimizations=OFF -Denable_compile_warnings=ON -Denable_lib_static=OFF -Denable_model-checking=OFF -Denable_latency_bound_tracking=OFF -Denable_gtnets=OFF -Denable_jedule=OFF -Denable_mallocators=OFF -Denable_memcheck=OFF -Denable_memcheck_xml=OFF -Denable_coverage=ON $WORKSPACE

make
ctest -D ExperimentalStart
ctest -D ExperimentalConfigure
ctest -D ExperimentalBuild
ctest -D ExperimentalTest
ctest -D ExperimentalCoverage

if [ -f Testing/TAG ] ; then
   /usr/local/gcovr-3.1/scripts/gcovr -r .. --xml-pretty -o $WORKSPACE/xml_coverage.xml 
   xsltproc $WORKSPACE/buildtools/jenkins/ctest2junit.xsl Testing/`head -n 1 < Testing/TAG`/Test.xml > CTestResults_memcheck.xml
   mv CTestResults_memcheck.xml $WORKSPACE
fi

