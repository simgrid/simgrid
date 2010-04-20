#! /bin/sh
echo "\n"
export CLASSPATH=$3:$3/src/simgrid.jar:$CLASSPATH
export LD_LIBRARY_PATH=$3/src/.libs:$LD_LIBRARY_PATH
cd $1
echo "CLASSPATH 	= $CLASSPATH"
echo "LD_LIBRARY_PATH 	= $LD_LIBRARY_PATH"
pwd
echo "java $2 *platform.xml *deployment.xml"
java $2 *platform.xml *deployment.xml
