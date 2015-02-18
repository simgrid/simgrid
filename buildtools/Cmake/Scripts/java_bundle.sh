#!/bin/sh

set -e
#set -x

if [ $# -lt 3 ]; then
    cat >&2 <<EOF
Usage: $0 simgrid.jar java_command strip_command [-so file.so...] [-txt file.txt...]
    simgrid.jar    SimGrid jar file
    java_command   path to the Java runtime
    strip_command  path to the command used to strip libraries
    file.so        library file to strip and bundle into the archive
    file.txt       other file to bundle into the archive
EOF
    exit 1
fi

SIMGRID_JAR=$1
JAVA=$2
STRIP=$3
shift 3

JSG_BUNDLE=$("$JAVA" -classpath "$SIMGRID_JAR" org.simgrid.NativeLib --quiet)

# sanity check
case "$JSG_BUNDLE" in
    NATIVE/*)
        cat >&2 <<EOF
-- [Java] Native libraries bundled into: ${JSG_BUNDLE}
EOF
        ;;
    *)
        cat >&2 <<EOF
-- [Java] Native libraries NOT bundled into invalid directory: ${JSG_BUNDLE}
EOF
        exit 1
        ;;
esac

# prepare directory
rm -fr NATIVE
mkdir -p "$JSG_BUNDLE"

if [ "$1" = "-so" ]; then
    shift
    for file; do
        [ "$file" != "-txt" ] || break
        cp -f "$file" "$JSG_BUNDLE"
        "$STRIP" -S "$JSG_BUNDLE/${file##*/}"
        shift
    done
fi

if [ "$1" = "-txt" ]; then
    shift
    for file; do
        cp -f "$file" "$JSG_BUNDLE"
        shift
    done
fi
