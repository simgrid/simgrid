#!/usr/bin/env python

# Copyright (c) 2011-2019. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

# This script generates a specific deployment file for the Chord example.
# It assumes that the platform will be a cluster.
# Usage: python generate.py nb_nodes nb_bits end_date
# Example: python generate.py 100000 32 1000

import sys
import random

if len(sys.argv) != 4:
    print(
        "Usage: python generate.py nb_nodes nb_bits end_date > deployment_file.xml")
    sys.exit(1)

nb_nodes = int(sys.argv[1])
nb_bits = int(sys.argv[2])
end_date = int(sys.argv[3])

max_id = 2 ** nb_bits - 1
all_ids = [42]

sys.stdout.write("<?xml version='1.0'?>\n"
                 "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">\n"
                 "<platform version=\"3\">\n"
                 "  <process host=\"node-0.simgrid.org\" function=\"node\"><argument value=\"42\"/><argument value=\"%d\"/></process>\n" % end_date)

for i in range(1, nb_nodes):

    ok = False
    while not ok:
        my_id = random.randint(0, max_id)
        ok = not my_id in all_ids

    known_id = all_ids[random.randint(0, len(all_ids) - 1)]
    start_date = i * 10
    line = "  <process host=\"node-%d.simgrid.org\" function=\"node\"><argument value=\"%d\" /><argument value=\"%d\" /><argument value=\"%d\" /><argument value=\"%d\" /></process>\n" % (
        i, my_id, known_id, start_date, end_date)
    sys.stdout.write(line)
    all_ids.append(my_id)

sys.stdout.write("</platform>")
