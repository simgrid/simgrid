#!/bin/sh

# Test this script locally as follows (rerun `docker pull simgrid/unstable` to get a fresh version).
# cd (simgrid)/tools/jenkins
# docker run -it --rm --volume `pwd`:/source simgrid/unstable /source/ci-bigdft.sh

set -ex

echo "XXXXXXXXXXXXXXXX Install APT dependencies"
SUDO="" # to ease the local testing
$SUDO apt-get -y update
$SUDO apt-get -y install git build-essential gfortran python-is-python3 python3-six python3-distutils python3-setuptools pkg-config automake cmake libboost-dev libblas-dev liblapack-dev wget

echo "XXXXXXXXXXXXXXXX build and test BigDFT (git version)"
git clone --depth=1 https://gitlab.com/l_sim/bigdft-suite.git
cd bigdft-suite

WORKSPACE=`pwd`
mkdir build && cd build
export PATH=$PWD/simgrid-dev/smpi_script/bin/:$PATH
export LD_LIBRARY_PATH=$PWD/simgrid-dev/lib/:$LD_LIBRARY_PATH
export JHBUILD_RUN_AS_ROOT=1

../Installer.py autogen -y

../Installer.py -f ../../tools/jenkins/gfortran-simgrid.rc -y build

export OMP_NUM_THREADS=1

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
