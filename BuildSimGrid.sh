#!/usr/bin/env sh
#
# This little script rebuilds and runs the SimGrid archive in parallel, extracting a log
# This is almost an internal script, but others may find this useful
#
# Copyright (c) 2017-2019 The SimGrid Team. Licence: LGPL of WDFPL, as you want.

if [ ! -e Makefile ] ; then
  if [ -e build/default/Makefile ] ; then
    cd build/default
  else
    echo "Please configure SimGrid before building it:"
    echo "   ccmake ."
    exit 1
  fi
fi

target=tests

install_path=$(sed -n 's/^CMAKE_INSTALL_PREFIX:PATH=//p' CMakeCache.txt)
if [ -e ${install_path} ] && [ -d ${install_path} ] && [ -x ${install_path} ] && [ -w ${install_path} ] ; then
  target=install
fi

(
  echo "install_path: ${install_path}"
  echo "Target: ${target}"
  (nice make -j4 ${target} tests || make ${target} tests) && nice ctest -j4 --output-on-failure ; date
) 2>&1 | tee BuildSimGrid.sh.log

