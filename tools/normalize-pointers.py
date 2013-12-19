#!/usr/bin/python
import sys, re

if len(sys.argv)!=2:
  print "Usage ./normalize-pointers.py <filename>"
  sys.exit(1)

f = open(sys.argv[1])
t = f.read()
f.close()

r = re.compile(r"0x[0-9a-f]{7}")
s = r.search(t)
offset = 0
pointers = {}
while (s):
  if s.group() not in pointers:
    pointers[s.group()] = "0X%07d"%len(pointers)
  print t[offset:s.start()],
  print pointers[s.group()],
  offset = s.end()
  s = r.search(t, offset)

print t[offset:]




