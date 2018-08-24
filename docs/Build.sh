#! /bin/sh
#
# Simplistic script to rebuild our documentation with sphinx-build

set -e

if [ "x$1" != 'xdoxy' -a -e build/xml ] ; then
  echo "(Doxygen not rerun)"
else
  rm -rf build/xml source/api/
  cd source; doxygen; cd ..
fi

sphinx-build -M html source build ${SPHINXOPTS}
cat source/img/graphical-toc.svg \
 | perl -pe 's/(xlink:href="http)/target="_top" $1/' \
 | perl -pe 's/(xlink:href=".*?.html)/target="_top" $1/' \
 > build/html/graphical-toc.svg
