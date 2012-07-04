#!/usr/bin/python

import sys, random

if len(sys.argv) != 4:
	print("Usage: python generate.py nb_nodes nb_bits end_date > deployment_file.xml")
	sys.exit(1)

nb_nodes = int(sys.argv[1])
nb_bits = int(sys.argv[2])
end_date = int(sys.argv[3])

max_id = 2 ** nb_bits - 1
all_ids = [0]

sys.stdout.write("<?xml version='1.0'?>\n"
"<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid.dtd\">\n"
"<platform version=\"3\">\n"
"  <process host=\"c-0.me\" function=\"node\"><argument value=\"0000000000000000000000000000000000000000\"/><argument value=\"%d\"/></process>\n" % end_date)

for i in range(1, nb_nodes):
	ok = False
	while not ok:
		my_id = random.randint(0, max_id)
		ok = not my_id in all_ids
	known_id = all_ids[random.randint(0, len(all_ids) - 1)]
	start_date = i * 10
	line = "  <process host=\"c-%d.me\" function=\"node\"><argument value=\"%s\" /><argument value=\"%s\" /><argument value=\"%d\" /></process>\n" % (i, my_id, known_id,end_date)
	sys.stdout.write(line)
	all_ids.append(my_id)

sys.stdout.write("</platform>")

