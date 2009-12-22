#! /bin/bash
# This script waits that the make_dist script finishes building the right archive in ~/simgrid


source ~/simgrid-svn/buildtools/Cmake/cmake_simgrid.conf

cd ${SIMGRID_SVN_ROOT}
svn up
version="simgrid-3.3.4-svn-r"`svnversion`

while [ ! -e ${SIMGRID_BASEDIR}/${version}.tar.gz ] ; do
  sleep 1
done