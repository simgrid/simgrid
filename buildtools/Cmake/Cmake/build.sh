#! /bin/sh

set -e
cd ..
if [ ! -e configure ] ; then
  ./bootstrap
fi

if [ ! -e Makefile ] ; then
  ./configure --enable-maintainer-mode --disable-compile-optimizations
fi

make