#!/usr/bin/env sh
#
# This little script rebuilds and runs the SimGrid archive in parallel, extracting a log
# This is almost an internal script, but others may find this useful
#
# Copyright (c) 2017-2018 The SimGrid Team. Licence: LGPL of WDFPL, as you want.

if [ ! -e Makefile ] ; then
  echo "Please configure SimGrid before building it:"
  echo "   ccmake ."
  exit 1
fi

(
  (nice make -j4 || make) && nice ctest -j4 --output-on-failure ; date
) 2>&1 | tee BuildSimGrid.sh.log
exit 0
