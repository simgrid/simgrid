#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys, re
from lxml import etree

xml = etree.parse(sys.argv[1])
for e in xml.xpath('//*[@routing="RuleBased"]'):
  e.attrib['routing'] = "Full"
  ids = e.xpath('./*[@id]/@id')
  done = set()  
  for asr in e.xpath("./ASroute"):
    src_ids = {}
    dst_ids = {}
    for id in ids:
      src_s = re.search(r"%s"%asr.attrib['src'], id)
      dst_s = re.search(r"%s"%asr.attrib['dst'], id)
      if src_s  :
        src_ids[id] = src_s
      if dst_s:
        dst_ids[id] = dst_s
    for sid, smat in src_ids.items():
      for did, dmat in dst_ids.items():
        todo = tuple(sorted((smat.group(1), dmat.group(1))))
        if todo not in done or asr.attrib.get("symmetrical")=="NO":
          done.add(todo)
          dasr = etree.tounicode(asr)
          dasr = dasr.replace("$1src", smat.group(1))
          dasr = dasr.replace("$1dst", dmat.group(1))
          dasr = etree.fromstring(dasr)
          dasr.tag = "__ASroute__"
          dasr.attrib['src'] = sid
          dasr.attrib['dst'] = did
          asr.addnext(dasr)
    asr.getparent().remove(asr)

print etree.tounicode(xml).replace("__ASroute__", "ASroute")

