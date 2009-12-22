#! /bin/sh
# This script goes to the directory specified as first argument, and executes the rest of its arguments
# It ought to be a solution to specify the PWD of commands in CTest, but I didn't find it yet


cd $1
shift
exec "$@"
