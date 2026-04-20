#! /bin/sh

# Test this script locally as follows (rerun `docker pull simgrid/unstable` to get a fresh version).
# cd (simgrid)/tools/jenkins
# docker run -it --rm --volume `pwd`:/source simgrid/unstable /source/ci-batsim.sh

set -ex

echo "XXXXXXXXXXXXXXXX Install APT dependencies"

export SUDO=""
$SUDO apt-get update
$SUDO apt-get -y install gcc g++ git

# Dependencies of BatSim
# OK  simgrid-unstable
# SRC intervalset-1.2.0
# SRC batprotocol-1.0
# PKG boost
# PKG rapidjson-1.1.0
# PKG zeromq-4.3.4
# PKG CLI11

# Dependencies of Batprotocol
# PKG flatbuffers

$SUDO apt-get -y install meson pkg-config rapidjson-dev libgtest-dev libzmq3-dev libboost-all-dev libcli11-dev libflatbuffers-dev

echo "XXXXXXXXXXXXXXXX Install intervalset"
git clone --depth 1 https://framagit.org/batsim/intervalset.git
cd intervalset
meson build
meson compile -C build && meson install -C build
cd ..

echo "XXXXXXXXXXXXXXXX Install batprotocol"
git clone --depth 1 https://framagit.org/batsim/batprotocol.git
cd batprotocol/cpp
meson build
meson compile -C build && meson install -C build
cd ../..

echo "XXXXXXXXXXXXXXXX Install and test batsim"
# install BatSim from their upstream git into the batsim.git directory
git clone --depth 1 https://framagit.org/batsim/batsim.git
cd batsim
meson build
meson compile -C build
