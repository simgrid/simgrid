#! /bin/bash
# This script waits that the make_dist script finishes building the right archive in ~/simgrid
# and then builds it using the autotools

if [ -e ~/simgrid-svn/buildtools/scripts/simgrid_build.conf ] ; then
  source ~/simgrid-svn/buildtools/scripts/simgrid_build.conf
else
  source ~/.simgrid_build.conf
fi

open_archive
if [ ! -e Makefile ] ; then
  ./configure $@
fi
./checkall
