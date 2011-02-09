#! /bin/bash
export LD_LIBRARY_PATH=$3/lib:$LD_LIBRARY_PATH:${SIMGRID_ROOT}/lib
export JAVA_LIBRARY_PATH=:$3/lib:$JAVA_LIBRARY_PATH:${SIMGRID_ROOT}/lib
cd $1
echo "LD_LIBRARY_PATH   = $LD_LIBRARY_PATH"
echo "JAVA_LIBRARY_PATH = $JAVA_LIBRARY_PATH"
pwd
echo "java -cp .:$3/simgrid.jar $2 *platform.xml $4*Deployment.xml"
java -cp .:$3/simgrid.jar $2 *platform.xml $4*Deployment.xml
