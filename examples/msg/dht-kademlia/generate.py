#!/usr/bin/env python

# Copyright (c) 2012-2019. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

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
all_ids = [0]

sys.stdout.write("<?xml version='1.0'?>\n"
                 "<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">\n"
                 "<platform version=\"4\">\n  <process host=\"node-0.simgrid.org\" function=\"node\">\n"
                 "     <argument value=\"0\"/>\n     <argument value=\"%d\"/>\n  </process>\n" % end_date)

for i in range(1, nb_nodes):
    ok = False
    while not ok:
        my_id = random.randint(0, max_id)
        ok = not my_id in all_ids
    known_id = all_ids[random.randint(0, len(all_ids) - 1)]
    start_date = i * 10
    line = "  <process host=\"node-%d.simgrid.org\" function=\"node\">\n    <argument value=\"%s\"/>"\
           "\n    <argument value=\"%s\"/>\n    <argument value=\"%d\"/>\n  </process>\n" % (
               i, my_id, known_id, end_date)
    sys.stdout.write(line)
    all_ids.append(my_id)

sys.stdout.write("</platform>")
