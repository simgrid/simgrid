#!/usr/bin/env sh
#
# This little script rebuilds and runs the SimGrid archive in parallel, extracting a log
# This is almost a personal script, but others may find this useful
#
# Copyright (c) 2017-2022 The SimGrid Team. Licence: LGPL of WDFPL, as you want.

if [ ! -e Makefile ] && [ ! -e build.ninja ]; then
  if [ -e build/default/Makefile ] ; then
    cd build/default
  else
    echo "Please configure SimGrid before building it:"
    echo "   ccmake ."
    exit 1
  fi
fi

target=tests
ncores=$(grep -c processor /proc/cpuinfo)

install_path=$(sed -n 's/^CMAKE_INSTALL_PREFIX:PATH=//p' CMakeCache.txt)
if [ -e ${install_path} ] && [ -d ${install_path} ] && [ -x ${install_path} ] && [ -w ${install_path} ] ; then
  target=install
fi

if [ -e build.ninja ] ; then
  builder="ninja"
else
  builder="make"
fi

(
  echo "install_path: ${install_path}"
  echo "Target: ${target}"
  echo "Cores: ${ncores}"
  (nice ${builder} -j${ncores} ${target} tests || make ${target} tests) && nice ctest -j${ncores} --output-on-failure ; date
) 2>&1 | tee BuildSimGrid.sh.log

