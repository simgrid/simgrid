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


echo "List of missing references:"
for f in `(grep '<name>' build/xml/msg_8h.xml; \
           grep '<name>' build/xml/namespacesimgrid_1_1s4u.xml; \
 	   grep '<innerclass refid=' build/xml/namespacesimgrid_1_1s4u.xml) |sed 's/<[^>]*>//g'|sort` 
do

  if grep $f source/*rst | grep -q '.. doxygen[^::]*:: '"$f"'$' ||
     grep $f source/*rst | grep -q '.. doxygen[^::]*:: simgrid::[^:]*::[^:]*::'"$f"'$'  ; then :   
#    echo "$f documented"
  else 
    if grep -q $f ignored_symbols ; then : 
#      echo "$f ignored" # not documented
    else
      echo "$f"
    fi
  fi
done