#! /bin/sh
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$3/lib
export JAVA_LIBRARY_PATH=$JAVA_LIBRARY_PATH:$3/lib
cd $1
echo "LD_LIBRARY_PATH   = $LD_LIBRARY_PATH"
echo "JAVA_LIBRARY_PATH = $JAVA_LIBRARY_PATH"
pwd
echo "java -cp .:$3/simgrid.jar $2 *platform.xml *deployment.xml"
java -cp .:$3/simgrid.jar $2 *platform.xml *deployment.xml
