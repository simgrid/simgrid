export CXX="g++"
export CC="gcc"
export SUDO=""

# Update refs, just in case
$SUDO apt-get update

# Install basic tools
$SUDO apt-get -y install cmake
$SUDO apt-get -y install gcc
$SUDO apt-get -y install g++
$SUDO apt-get -y install unzip
$SUDO apt-get -y install doxygen
$SUDO apt-get -y install wget
$SUDO apt-get -y install git
$SUDO apt-get -y install libboost-all-dev
$SUDO apt-get -y install libpugixml-dev
$SUDO apt-get -y install nlohmann-json3-dev

# install googletest
wget https://github.com/google/googletest/archive/release-1.8.0.tar.gz && tar xf release-1.8.0.tar.gz && cd googletest-release-1.8.0/googletest && cmake . && make && $SUDO make install && cd ../.. && rm -rf release-1.8.0.tar.gz googletest-release-1.8.0

set -e
# install WRENCH from their upstream git into the wrench.git directory
rm -rf wrench.git && git clone --depth 1 --branch simgrid-external-project-ci https://github.com/wrench-project/wrench.git wrench.git
(mkdir wrench.git/build && cd wrench.git/build && cmake -DSIMGRID_INSTALL_PATH=/usr/ .. && make unit_tests && ./unit_tests && cd ../.. && rm -rf wrench.git) || exit 1



