#!/usr/bin/env sh

die() {
    echo "$@"
    exit 1
}

[ -n "$WORKSPACE" ] || die "No WORKSPACE"
[ -d "$WORKSPACE" ] || die "WORKSPACE ($WORKSPACE) does not exist"

echo "XXXX Cleanup previous attempts. Remaining content of /tmp:"
rm -f /tmp/cc*
rm -f /tmp/simgrid-mc-*
rm -f /tmp/*.so
rm -f /tmp/*.so.*
ls /tmp
df -h
echo "XXXX Let's go"

set -e

BUILDFOLDER=$WORKSPACE/build

### Check the node installation

pkg_check() {
  for pkg
  do
    if command -v "$pkg"
    then
       echo "$pkg is installed. Good."
    else
       die "please install $pkg before proceeding"
    fi
  done
}

pkg_check xsltproc gcovr ant cover2cover.py

### Cleanup previous runs

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

do_cleanup "$BUILDFOLDER"

NUMPROC="$(nproc)" || NUMPROC=1


cd "$BUILDFOLDER"
rm -rf python_cov*
rm -rf xml_coverage.xml

ctest -D ExperimentalStart || true

cmake -Denable_documentation=OFF \
      -Denable_compile_optimizations=OFF -Denable_compile_warnings=ON \
      -Denable_mallocators=ON \
      -Denable_ns3=ON \
      -Denable_smpi=ON -Denable_testsuite_smpi_MPICH3=ON -Denable_model-checking=ON \
      -Denable_smpi_papi=ON \
      -Denable_memcheck=OFF -Denable_memcheck_xml=OFF -Denable_testsuite_smpi_MBI=ON -Denable_testsuite_McMini=ON \
      -Denable_coverage=ON -DLTO_EXTRA_FLAG="auto" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON "$WORKSPACE"

#build with sonarqube scanner wrapper
/home/ci/build-wrapper-linux-x86/build-wrapper-linux-x86-64 --out-dir bw-outputs make -j$NUMPROC tests

export PYTHON_TOOL_OPTIONS="/usr/bin/python3-coverage run --parallel-mode --branch"

ctest --no-compress-output -D ExperimentalTest -j$NUMPROC || true
ctest -D ExperimentalCoverage || true

if [ -f Testing/TAG ] ; then

  #convert python coverage reports in xml ones
  cd "$BUILDFOLDER"
  find .. -size +1c -name ".coverage*" -exec mv {} . \;
  /usr/bin/python3-coverage combine
  /usr/bin/python3-coverage xml -i -o ./python_coverage.xml

  #convert all gcov reports to xml cobertura reports
  gcovr -r "$WORKSPACE" --xml-pretty -e "$WORKSPACE"/teshsuite -e MBI -e "$WORKSPACE"/examples/smpi/NAS -e "$WORKSPACE"/examples/smpi/mc -u -o xml_coverage.xml --gcov-ignore-parse-errors all --gcov-ignore-errors all

  cd "$WORKSPACE"
  xsltproc "$WORKSPACE"/tools/jenkins/ctest2junit.xsl build/Testing/"$( head -n 1 < build/Testing/TAG )"/Test.xml > CTestResults_memcheck.xml

  #generate sloccount report
  sloccount --duplicates --wide --details "$WORKSPACE" | grep -v -e '.git' -e 'mpich3-test' -e 'sloccount.sc' -e 'build/' -e 'xml_coverage.xml' -e 'CTestResults_memcheck.xml' -e 'DynamicAnalysis.xml' > "$WORKSPACE"/sloccount.sc

  #generate PVS-studio report
  EXCLUDEDPATH="-e $WORKSPACE/src/3rd-party/catch.hpp -e $WORKSPACE/src/3rd-party/xxhash.hpp -e $WORKSPACE/teshsuite/smpi/mpich3-test/ -e *_dtd.c -e *_dtd.h -e *.yy.c -e *.tab.cacc -e *.tab.hacc -e $WORKSPACE/src/smpi/colls/ -e $WORKSPACE/examples/smpi/NAS/ -e $WORKSPACE/examples/smpi/gemm/gemm.cq"
  pvs-studio-analyzer analyze -f "$BUILDFOLDER"/compile_commands.json -o "$WORKSPACE"/pvs.log $EXCLUDEDPATH -j$NUMPROC
  # Disable:
  # V521 Such expressions using the ',' operator are dangerous. (-> commas in catch.hpp),
  # V576 Incorrect format. (-> gives false alarms, and already checked elsewhere)
  # V1042 This file is marked with copyleft license, which requires you to open the derived source code.
  # V1056 The predefined identifier '__func__' always contains the string 'operator()' inside function body of the overloaded 'operator()'.
  plog-converter -t xml -o "$WORKSPACE"/pvs.plog -d V521,V576,V1042,V1056 "$WORKSPACE"/pvs.log

fi || exit 42
