#! /bin/sh

set -e
cd ../..
if [ ! -e configure ] ; then
  ./bootstrap
fi

if [ ! -e Makefile ] ; then
  ./configure --enable-maintainer-mode --disable-compile-optimizations
fi

make
make -C testsuite xbt/log_usage xbt/heap_bench xbt/graphxml_usage 
make -C testsuite surf/maxmin_bench surf/lmm_usage surf/trace_usage surf/surf_usage surf/surf_usage2
make -C testsuite simdag/sd_test