#!/bin/bash

if [ -z $1 ]
then
  echo "Usage: $0 {X.trace}"
  exit
fi

TRACE=$1
echo "input: $TRACE"
OUTPUT=`echo $TRACE | cut -d\. -f1`.fix.trace

cat $TRACE | grep ^% > header
DEFEVENTS=`cat header | grep Define | awk '{ print $3 }'`

GREP=""
GREP2=""
for i in $DEFEVENTS
do
  GREP="/^$i /d; $GREP"
  GREP2="-e '^$i ' $GREP2"
done
GREP="/^%\ /d; /^%E/d; $GREP"

cat $TRACE | eval grep $GREP2 > types
/bin/sed -e "$GREP" $TRACE > events
cat events |  sort -n -k 2 -s > events.sorted
cat header types events.sorted > $OUTPUT
rm types events events.sorted header

echo "output: $OUTPUT"
