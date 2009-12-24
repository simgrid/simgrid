#!/bin/bash
# This script creates a new dist archive from the svn

set -e # fail fast on errors 

# get config
if [ -e ~/simgrid-svn/buildtools/scripts/simgrid_build.conf ] ; then
  source ~/simgrid-svn/buildtools/scripts/simgrid_build.conf
else
  source ~/.simgrid_build.conf
fi

if [ ! -e /usr/bin/libtool ] || [ ! -e /usr/bin/automake ] || [ ! -e /usr/bin/flex ] ; then
  echo "get Linux dependencies"
  sudo aptitude install -y libtool automake1.10 autoconf libgcj10-dev gcc g++ bash flex flexml doxygen bibtex bibtool iconv bibtex2html addr2line valgrind transfig || true
fi

make_dist

echo "Done!"
