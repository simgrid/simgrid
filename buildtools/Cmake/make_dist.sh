#!/bin/bash
# This script creates a new dist archive from the svn

set -e # fail fast on errors 
source ~/simgrid-svn/buildtools/Cmake/cmake_simgrid.conf # get config

echo "get Linux dependencies"
sudo aptitude install -y libtool automake1.10 autoconf libgcj10-dev gcc g++ bash flex flexml doxygen bibtex bibtool iconv bibtex2html addr2line valgrind transfig

make_dist

echo "Done!"
