#! /bin/sh
echo "\n"
export CLASSPATH=$3:$3/src/simgrid.jar:$CLASSPATH
cd $1
echo "CLASSPATH = $CLASSPATH"
pwd
echo "java $2 *platform.xml *deployment.xml"
java $2 *platform.xml *deployment.xml
