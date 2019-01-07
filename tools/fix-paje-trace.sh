#!/usr/bin/env bash

# Copyright (c) 2010-2019. The SimGrid Team. All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

if [ -z $1 ]
then
  echo "Usage: $0 {X.trace}"
  exit
fi

TRACE=$1
echo "input: $TRACE"
OUTPUT=$( echo $TRACE | cut -d\. -f1 ).fix.trace

grep ^% < $TRACE > header
DEFEVENTS=$(grep Define < header | awk '{ print $3 }')

GREP=""
GREP2=""
for i in $DEFEVENTS
do
  GREP="/^$i /d; $GREP"
  GREP2="-e '^$i ' $GREP2"
done
GREP="/^%\ /d; /^%	/d; /^%E/d; $GREP"

grep $GREP2 < $TRACE > types
/bin/sed -e "$GREP" $TRACE > events
sort -n -k 2 -s < events > events.sorted
cat header types events.sorted > $OUTPUT
rm types events events.sorted header

echo "output: $OUTPUT"
