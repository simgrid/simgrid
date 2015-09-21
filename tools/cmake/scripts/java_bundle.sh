#!/bin/sh

set -e
set -x

if [ $# -lt 2 ]; then
    cat >&2 <<EOF
Usage: $0 simgrid.jar java_command strip_command [file.so...]
    java_command   path to the Java runtime
    simgrid.jar    SimGrid jar file
    file.so        library file to strip and bundle into the archive
EOF
    exit 1
fi

JAVA=$1
shift
SIMGRID_JAR=$1
shift

JSG_BUNDLE=$("$JAVA" -classpath "$SIMGRID_JAR" org.simgrid.NativeLib --quiet)

# prepare directory
rm -fr NATIVE
mkdir -p "$JSG_BUNDLE"
cp $* "$JSG_BUNDLE"

