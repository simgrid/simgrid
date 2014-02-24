#!/bin/bash

# Copyright (c) 2011, 2014. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

INFILE="$1"
OUTFILE="$2"
SPFILE="replace_globals.cocci"
TMPFILE=`mktemp ${OUTFILE}.XXXX`

trap "rm -f ${TMPFILE}" EXIT
spatch -sp_file ${SPFILE} ${INFILE} -o ${TMPFILE} >/dev/null 2>/dev/null
./fixsrc.pl < ${TMPFILE} > ${OUTFILE}
