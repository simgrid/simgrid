#! /bin/sh
export LD_LIBRARY_PATH="../src/.libs:$LD_LIBRARY_PATH"
export CLASSPATH=".:../src/simgrid.jar:$CLASSPATH"
cd $1
if [ ! -e $2.class ] ; then
  rm classnoinst.stamp
  make classnoinst.stamp
fi
pwd
echo LD_LIBRARY_PATH="../../../src/.libs:$LD_LIBRARY_PATH" java -cp ".:../../../src/simgrid.jar:$CLASSPATH" $options $2 *platform.xml *deployment.xml
LD_LIBRARY_PATH="../../../src/.libs:$LD_LIBRARY_PATH" java -cp ".:../../../src/simgrid.jar:$CLASSPATH"  $options $2 *platform.xml *deployment.xml