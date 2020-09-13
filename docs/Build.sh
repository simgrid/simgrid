#! /bin/bash
#
# Simplistic script to rebuild our documentation with sphinx-build

# If you are missing some dependencies, try: pip3 install --requirement docs/requirements.txt

# Python needs to find simgrid on my machine, but not ctest -- sorry for the hack
if [ -e /opt/simgrid ] ; then chmod +x /opt/simgrid; fi

set -ex
set -o pipefail

if [ "x$1" != 'xdoxy' ] && [ -e build/xml ] ; then
  echo "Doxygen not rerun: 'doxy' was not provided as an argument"
else
  rm -rf build/xml source/api/
  (cd source; doxygen 2>&1; cd ..) | grep -v "is not documented." #   XXXXX Reduce the verbosity for now
fi

if [ "x$1" != 'xjava' ] && [ -e source/java ] ; then
  echo "javasphinx not rerun: 'java' was not provided as an argument"
else
  rm -rf source/java
  
  # Use that script without installing javasphinx: javasphinx-apidoc --force -o source/java/ ../src/bindings/java/org/simgrid/msg
  PYTHONPATH=${PYTHONPATH}:source/_ext/javasphinx python3 - --force -o source/java/ ../src/bindings/java/org/simgrid/msg <<EOF
import re
import sys
from javasphinx.apidoc import main
if __name__ == '__main__':
    sys.argv[0] = re.sub(r'(-script\.pyw|\.exe)?$', '', sys.argv[0])
    sys.exit(main())
EOF

  rm -f source/java/packages.rst # api_generated/source_java_packages.rst
  rm -f source/java/org/simgrid/msg/package-index.rst # api_generated/source_java_org_simgrid_msg_package-index.rst
  for f in source/java/org/simgrid/msg/* ; do
    # Add the package name to the page titles
    (printf "class org.simgrid.msg."; cat $f )>tmp
    mv tmp $f
    sed -i 's/==/========================/' $f # That's the right length knowing that I add 'class org.simgrid.msg.'
  done
#  sed -i 's/^.. java:type:: public class /.. java:type:: public class org.simgrid.msg/' source/java/org/simgrid/msg/*
  echo "javasphinx relaunched"
fi

PYTHONPATH=../lib:source/_ext/javasphinx sphinx-build -M html source build ${SPHINXOPTS} 2>&1 \
  | grep -v 'WARNING: cpp:identifier reference target not found: simgrid$' \
  | grep -v 'WARNING: cpp:identifier reference target not found: simgrid::s4u$' \
  | grep -v 'WARNING: cpp:identifier reference target not found: boost' 

set +x

perl -pe 's/(xlink:href="(?:http|.*\.html))/target="_top" $1/' \
     source/img/graphical-toc.svg > build/html/graphical-toc.svg

echo
echo "Undocumented examples:"
for ex in $( (cd .. ; \
              find examples/s4u/ -name '*.cpp'; \
              find examples/c/ -name '*.c'; \
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
    linkchecker --no-status -o csv --ignore-url='.*\.css$' --ignore-url=build/html/_modules  --ignore-url=public/java/org build/html \
     | grep -v '^#' \
     | grep -v 'urlname;parentname;baseref;result;warningstring'
  echo "done."
else
  echo "Install linkchecker to have it executed when you build the doc."
fi

