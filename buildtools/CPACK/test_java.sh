#! /bin/sh
echo "\n\n"
pwd
export LD_LIBRARY_PATH="`pwd`/src/.libs:$LD_LIBRARY_PATH"
export CLASSPATH="`pwd`/src/simgrid.jar:$CLASSPATH"
cd $1
export CLASSPATH="$CLASSPATH:`pwd`"
pwd

echo "LD_LIBRARY_PATH = $LD_LIBRARY_PATH"
echo "CLASSPATH = $CLASSPATH"

echo "\n\n"
echo "java $2 *platform.xml *deployment.xml"
echo "\n\n"
java $2 *platform.xml *deployment.xml
