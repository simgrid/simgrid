#! /bin/sh
#
# Simplistic script to rebuild our documentation with sphinx-build

rm -rf build/doxy/ source/api/
sphinx-build -M html source build ${SPHINXOPTS}
cat source/img/graphical-toc.svg \
 | perl -pe 's/(xlink:href="http)/target="_top" $1/' \
 | perl -pe 's/(xlink:href=".*?.html)/target="_top" $1/' \
 > build/html/graphical-toc.svg
