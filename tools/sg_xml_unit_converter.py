#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2013-2019. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

# grep -ohrI 'bw=".*"' . | sort | uniq

import sys
import fnmatch
import os
from decimal import Decimal, DecimalException
import re


def to_str(dec):
    return re.sub(r"(\.\d*?)0*$", r"\1", dec.to_eng_string()).rstrip(".")


def convert(xml, formats, attrib):
    res = []
    m = re.search(r'%s="(.*?)"' % attrib, xml)
    while m:
        b, e = m.span(1)
        res.append(xml[:b])
        val = xml[b:e]
        xml = xml[e:]
        try:
            power = Decimal(val)
            tmp = to_str(power)
            for p, f in formats:
                d = power / p
                if d >= 1.0:
                    tmp = "%s%s" % (to_str(d), f)
                    break
            res.append(tmp)
        except DecimalException:
            print "Error with:", val
            res.append(val)
        m = re.search(r'%s="(.*?)"' % attrib, xml)

    res.append(xml)
    return "".join(res)


def formats(ll):
    return sorted(((Decimal(i), j) for i, j in ll), key=lambda x: x[0], reverse=True)


for root, dirnames, filenames in os.walk(sys.argv[1]):
    for filename in fnmatch.filter(filenames, '*.xml'):
        print root, dirnames, filename
        path = os.path.join(root, filename)
        xml = open(path).read()

        power_formats = formats([("1E0", "f"),
                                 ("1E3", "kf"),
                                 ("1E6", "Mf"),
                                 ("1E9", "Gf"),
                                 ("1E12", "Tf"),
                                 ("1E15", "Pt"),
                                 ("1E18", "Ef"),
                                 ("1E21", "Zf")])
        xml = convert(xml, power_formats, "power")

        bandwidth_formats = formats([("1E0", "Bps"),
                                     ("1E3", "kBps"),
                                     ("1E6", "MBps"),
                                     ("1E9", "GBps"),
                                     ("1E12", "TBps")])
        xml = convert(xml, bandwidth_formats, "bandwidth")
        xml = convert(xml, bandwidth_formats, "bw")
        xml = convert(xml, bandwidth_formats, "bb_bw")
        xml = convert(xml, bandwidth_formats, "bw_in")
        xml = convert(xml, bandwidth_formats, "bw_out")

        time_formats = formats([("1E-12", "ps"),
                                ("1E-9", "ns"),
                                ("1E-6", "us"),
                                ("1E-3", "ms"),
                                ("1E0", "s"),
                                ("60E0", "m"),
                                ("3600E0", "h"),
                                ("86400E0", "d"),
                                ("604800E0", "w")])
        xml = convert(xml, time_formats, "latency")
        xml = convert(xml, time_formats, "lat")
        xml = convert(xml, time_formats, "bb_lat")

        # print xml
        outfile = open(path, "w")
        outfile.write(xml)
        outfile.close()
