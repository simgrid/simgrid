#!/bin/bash
INFILE="$1"
OUTFILE="$2"
SPFILE="replace_globals.cocci"
TMPFILE=`mktemp ${OUTFILE}.XXXX`

trap "rm -f ${TMPFILE}" EXIT
spatch -sp_file ${SPFILE} ${INFILE} -o ${TMPFILE} >/dev/null 2>/dev/null
./fixsrc.pl < ${TMPFILE} > ${OUTFILE}
