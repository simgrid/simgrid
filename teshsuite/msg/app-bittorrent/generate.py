#!/usr/bin/env python

# Copyright (c) 2012-2019. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

# This script generates a specific deployment file for the Bittorrent example.
# It assumes that the platform will be a cluster.
# Usage: python generate.py nb_nodes nb_bits end_date percentage
# Example: python generate.py 10000 5000

import sys
import random

if len(sys.argv) != 4:
    print(
        "Usage: python generate.py nb_nodes end_date seed_percentage > deployment_file.xml")
    sys.exit(1)

nb_nodes = int(sys.argv[1])
end_date = int(sys.argv[2])
seed_percentage = int(sys.argv[3])

nb_bits = 24
max_id = 2 ** nb_bits - 1
all_ids = [42]

sys.stdout.write("<?xml version='1.0'?>\n"
                 "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">\n"
                 "<platform version=\"4\">\n"
                 "  <process host=\"node-0.simgrid.org\" function=\"tracker\">\n"
                 "    <argument value=\"%d\"/>\n  </process>\n" % end_date)

for i in range(1, nb_nodes):

    ok = False
    while not ok:
        my_id = random.randint(0, max_id)
        ok = not my_id in all_ids
    start_date = i * 10
    line = "  <process host=\"node-%d.simgrid.org\" function=\"peer\">\n" % i
    line += "    <argument value=\"%d\"/>\n    <argument value=\"%d\"/>\n" % (
        my_id, end_date)
    if random.randint(0, 100) < seed_percentage:
        line += "    <argument value=\"1\"/>\n"
    line += "  </process>\n"
    sys.stdout.write(line)
    all_ids.append(my_id)
sys.stdout.write("</platform>")
