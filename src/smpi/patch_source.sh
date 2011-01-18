#!/bin/bash
INFILE="$1"
OUTFILE="$2"
SPFILE="replace_globals.cocci"
spatch -sp_file ${SPFILE} ${INFILE} 2>/dev/null | patch -o - | ./fixsrc.pl > ${OUTFILE}
