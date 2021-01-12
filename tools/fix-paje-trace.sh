#!/usr/bin/env bash

# Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

if [ -z $1 ]
then
  echo "Usage: $0 {X.trace}"
  exit
fi

TRACE=$1
echo "input: $TRACE"
OUTPUT=${TRACE%.*}.fix.trace

grep ^% -- "$TRACE" > header
DEFEVENTS=( $(awk '/Define/ { print $3 }' header) )

GREP=()
for i in "${DEFEVENTS[@]}"
do
  GREP+=( -e "^$i " )
done
GREP2=( -e $'^%[ \tE]' "${GREP[@]}" )

grep "${GREP[@]}" -- "$TRACE" > types
grep -v "${GREP2[@]}" -- "$TRACE" > events
sort -n -k 2 -s < events > events.sorted
cat header types events.sorted > "$OUTPUT"
rm -f types events events.sorted header

echo "output: $OUTPUT"
