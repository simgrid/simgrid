#! /bin/sh

# Test this script locally as follows (rerun `docker pull simgrid/unstable` to get a fresh version).
# cd <simgrid/tools/jenkins
# docker run -it --rm --volume `pwd`:/source simgrid/unstable /source/ci-batsim.sh

set -ex

echo "XXXXXXXXXXXXXXXX Install APT dependencies"

export SUDO=""
$SUDO apt-get update
$SUDO apt-get -y install gcc g++ git

# Dependencies of BatSim
# OK  simgrid-3.28
# SRC intervalset-1.2.0
# PKG rapidjson-1.1.0
# ??  redox
# PKG hiredis-1.0.0
# PKG zeromq-4.3.4
# PKG docopt.cpp-0.6.3
# PKG pugixml-1.11.1
# PKG gtest-1.10.0-dev
$SUDO apt-get -y install meson pkg-config libpugixml-dev libgtest-dev rapidjson-dev python3-hiredis libzmq3-dev libdocopt-dev libboost-all-dev

echo "XXXXXXXXXXXXXXXX Install intervalset"
git clone https://framagit.org/batsim/intervalset.git
cd intervalset 
meson build --prefix=/usr
cd build && ninja install
cd ../..

echo "XXXXXXXXXXXXXXXX Install redox"
$SUDO apt-get -y install libhiredis-dev libev-dev cmake #for redox
git clone --depth=1 --branch=install-pkg-config-file https://github.com/mpoquet/redox.git
cd redox 
cmake -DCMAKE_INSTALL_PREFIX=/usr -Dstatic_lib=OFF . && make install
cp redox.pc /usr/lib/pkgconfig/
cd ..

echo "XXXXXXXXXXXXXXXX Install and test batsim"
# install BatSim from their upstream git into the batsim.git directory
git clone --depth 1 https://gitlab.inria.fr/batsim/batsim
cd batsim
meson build -Ddo_unit_tests=true
ninja -C build
meson test -C build

echo "XXXXXXXXXXXXXXXX cat /batsim/build/meson-logs/testlog.txt"
cat build/meson-logs/testlog.txt
