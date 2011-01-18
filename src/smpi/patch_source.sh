#!/bin/bash
INFILE="$1"
OUTFILE="$2"
SPFILE="replace_globals.cocci"
spatch -sp_file ${SPFILE} $1 -o $2.tmp >/dev/null 2>/dev/null
./fixsrc.pl < $2.tmp > $2
rm $2.tmp
