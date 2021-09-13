#!/usr/bin/env sh
set -ex
export OMP_NUM_THREADS=1

SUDO="" # to ease the local testing
$SUDO apt-get -y update
$SUDO apt-get -y install git
$SUDO apt-get -y install build-essential
$SUDO apt-get -y install python-is-python3
$SUDO apt-get -y install python3-six
$SUDO apt-get -y install jhbuild

git clone --depth=1 https://gitlab.com/l_sim/bigdft-suite.git
cd bigdft-suite

WORKSPACE=`pwd`
mkdir build && cd build

../Installer.py autogen -y

../Installer.py -f ../../tools/jenkins/gfortran-simgrid.rc -y build

#cubic version
cd ../bigdft/tests/DFT/cubic/C
smpirun -hostfile $WORKSPACE/simgrid-dev/examples/smpi/hostfile -platform $WORKSPACE/simgrid-dev/examples/platforms/small_platform.xml -np 8 $WORKSPACE/build/install/bin/bigdft -l no

#Psolver checking with smpi_shared_malloc
cd $WORKSPACE/build/psolver/tests
make FC=smpif90 PS_Check
smpirun -hostfile $WORKSPACE/simgrid-dev/examples/smpi/hostfile -platform $WORKSPACE/simgrid-dev/examples/platforms/small_platform.xml -np 4 ./PS_Check -n [57,48,63] -g F

#linear scaling version (heavy, might swap)
cd $WORKSPACE/bigdft/tests/DFT/linear/surface
smpirun -hostfile $WORKSPACE/simgrid-dev/examples/smpi/hostfile -platform $WORKSPACE/simgrid-dev/examples/platforms/small_platform.xml -np 4 $WORKSPACE/build/install/bin/bigdft -n graphene -l no

cd $WORKSPACE/build
../Installer.py -f ../../tools/jenkins/gfortran-simgrid.rc -y clean
