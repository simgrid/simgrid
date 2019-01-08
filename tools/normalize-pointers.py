#!/usr/bin/env python

# Copyright (c) 2013-2019. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
Tool for normalizing pointers such as two runs have the same 'addresses'

first address encountered will be replaced by 0X0000001, second by 0X0000002, ...
"""

import sys
import re

if len(sys.argv) != 2:
    print "Usage ./normalize-pointers.py <filename>"
    sys.exit(1)

f = open(sys.argv[1])
t = f.read()
f.close()

r = re.compile(r"0x[0-9a-f]+")
s = r.search(t)
offset = 0
pointers = {}
while (s):
    if s.group() not in pointers:
        pointers[s.group()] = "0X%07d" % len(pointers)
    print t[offset:s.start()],
    print pointers[s.group()],
    offset = s.end()
    s = r.search(t, offset)

print t[offset:]
