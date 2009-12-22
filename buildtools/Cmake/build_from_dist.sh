#! /bin/bash
# This script waits that the make_dist script finishes building the right archive in ~/simgrid
# and then builds it using the autotools


source ~/simgrid-svn/buildtools/Cmake/cmake_simgrid.conf

build_from_autotools 
