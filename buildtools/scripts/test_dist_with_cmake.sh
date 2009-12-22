#! /bin/bash
# This script waits that the make_dist script finishes building the right archive in ~/simgrid
# and then builds it using cmake. Warning, cmake is only a wrapper for now, you still need the autotools

if [ -e ~/simgrid-svn/buildtools/scripts/simgrid_build.conf ] ; then
  source ~/simgrid-svn/buildtools/scripts/simgrid_build.conf
else
  source ~/.simgrid_build.conf
fi

set -e
open_archive

# Make sure we have cmake installed
which_cmake=`which cmake` 
if [ x$which_cmake = x ] ; then
  echo "Try to install cmake"
  sudo apt-get install cmake
fi

# Launch CMake
cd buildtools/Cmake
cmake ./

# Run CTest, and push the results
ctest -D Experimental CTEST_FULL_OUTPUT
