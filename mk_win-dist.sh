#! /bin/sh
set -x

# Detection of java broken in cross compile, force the result
if grep -q 'if test -n "$JAVAC"' configure ; then
  sed -i 's/.*if test -n "$JAVAC".*/if true ; then/' configure
fi

if [ -e win32 ] ; then
  cd win32 
else
  mkdir win32 || true
  cd win32
  # configure the cross compilation (lua broken in autotools)
  ../configure --host=i586-mingw32msvc --disable-lua --enable-maintainer-mode
fi

cd src
# cross compilation raises some warnings I want to ignore
sed -i s/-Werror// Makefile
make libsimgrid.la
cd ../..

# Prepare the archive
rm -rf simgrid-3.4.1-win32
mkdir simgrid-3.4.1-win32
cp win32/src/.libs/libsimgrid-2.dll simgrid-3.4.1-win32/simgrid.dll
cp src/simgrid.jar simgrid-3.4.1-win32/ 
cd simgrid-3.4.1-win32/
cp /usr/share/doc/mingw32-runtime/mingwm10.dll.gz .
gunzip mingwm10.dll.gz
cat <<EOF >README
This is the Windows port of the SimGrid Java bindings.


INSTALLATION
============

To use it, put the simgrid.jar somewhere in your classpath, and the 2
dlls (ie, simgrid.dll and mingw32.dll) in a directory used by your
system. The directory from which you launch the java machine should
work, as well as your system directory (c:\system or something).

INSTALLATION WITH ECLIPSE
=========================

You should put the 3 files simgrid.jar simgrid.dll and mingw32.dll in
a separate directory of your workspace (say, 'libs').

Then, edit the properties of your project, item "Java Build Path".
* Open the tab "Libraries" and click "Add JARs".
  Search for the simgrid.jar in your disk, and select it.
* Open the tab "Source". Unfold the source directory (little triangle).
  Click on "Native Library Location", and then the "Edit" button.
  Then, select the path to the dlls (workspace:libs if you followed me).

DOCUMENTATION
=============
Some examples are included in this archive, and the full documentation
is available here:
http://simgrid.gforge.inria.fr/doc/group__MSG__JAVA.html

LIMITATIONS
===========
The Java bindings should work as well on Windows than on any other
platform. If not, please report any bug you find at:
http://gforge.inria.fr/tracker/?atid=165&group_id=12

The C version may work, but you are on your own if you try to use it.
We will assist you if possible, but our time (and ability with
windows) are limited. 

In any case, any feedback is welcome on simgrid-user@listes.gforge.inria.fr

Enjoy, 
the SimGrid team.

EOF

cd ../examples/java
for n in `find -name '*java'` ; do
  mkdir --parent `dirname "../../simgrid-3.4.1-win32/examples/$n"`
  cp -p $n `dirname "../../simgrid-3.4.1-win32/examples/$n"`
done

for n in `find -name '*xml'` ; do
  cp -p $n `dirname "../../simgrid-3.4.1-win32/examples/$n"`
done

cd ../..
rm -f simgrid-3.4.1-win32.zip
zip -r simgrid-3.4.1-win32.zip simgrid-3.4.1-win32
rm -rf simgrid-3.4.1-win32