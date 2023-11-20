#! /bin/bash
#
# Copyright (c) 2018-2023. The SimGrid Team. All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

# Simplistic script to rebuild our documentation with sphinx-build

# If you are missing some dependencies, try: pip3 install --requirement docs/requirements.txt

# Python needs to find simgrid on my machine, but not ctest -- sorry for the hack
if [ -e /opt/simgrid ] ; then chmod +x /opt/simgrid; fi

set -e
set -o pipefail

if [ "x$1" != 'xdoxy' ] && [ -e build/xml ] ; then
  echo "Doxygen not rerun: 'doxy' was not provided as an argument"
else
  set -x
  rm -rf build/xml source/api/
  (cd source; doxygen 2>&1; cd ..) | (grep -v "is not documented." || true) # XXXXX Reduce the verbosity for now
  set +x
fi

if [ "x$1" != 'xlogs' ] && [ -e build/log_categories.rst ] ; then
  echo "Log categories not extracted: 'logs' was not provided as an argument"
else
  set -x
  perl ./bin/extract_logs_hierarchy.pl ../ > build/log_categories.rst
  set +x
fi

PYTHONPATH=../lib sphinx-build -M html source build ${SPHINXOPTS} 2>&1

set +x

perl -pe 's/(xlink:href="(?:http|.*\.html))/target="_top" $1/' \
     source/img/graphical-toc.svg > build/html/graphical-toc.svg

echo
echo "Undocumented examples:"
for ex in $( (cd .. ; \
              find examples/cpp/   -name '*.cpp'; \
              find examples/c/     -name '*.c'; \
              find examples/python -name '*.py'; \
             ) | sort )
do
    if grep -q "example-tab:: $ex" ../examples/README.rst ; then :
#        echo "found example-tab:: $ex"
    elif grep -q "showfile:: $ex" ../examples/README.rst ; then :
    else
        echo $ex
    fi
done

set +e # Don't fail
if [ -e /usr/bin/linkchecker ] ; then
    linkchecker --no-status -o csv --ignore-url='.*\.css$' --ignore-url=build/html/_modules build/html \
     | grep -v '^#' \
     | grep -v 'urlname;parentname;baseref;result;warningstring'
  echo "done."
else
  echo "Install linkchecker to have it executed when you build the doc."
fi

echo "Undocumented symbols:"
./find-missing.py 2>&1
