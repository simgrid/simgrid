#!/bin/sh

cd $(dirname $0)/../../

# -R -- we want to search recursively
# -l -- only filenames, nothing else
# $1 -- argument to this script, name of the routing model (e.g., "Floyd")
# .  -- search in the examples folder and search EVERYTHING
# --include -- but only include results that end in ".xml" 
grep -R -l "$1" examples/ --include "*.xml"

exit 0
