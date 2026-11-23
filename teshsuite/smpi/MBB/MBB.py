#! /usr/bin/env python3

# Copyright 2021-2026. The SimGrid Team. All rights reserved.
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

# Runs a compiled MBB test case under simgrid-mc, and checks that the error is detected iff the case expects one.
# The classification of the outputs comes from the simgrid.py tool of the MBI project.

import os
import re
import signal
import subprocess
import sys

if len(sys.argv) != 4:
    print(f"Usage: MBB.py build_dir binary source (received: {sys.argv})")
    sys.exit(1)
(_, build_dir, binary, source) = sys.argv

# Parse the header of the source: a '$ command' line followed by a '| OK' or '| ERROR <class>' line
with open(source, 'r') as infile:
    header = infile.read().split('END_MBB_TESTS')[0]
m = re.search(r'\n\s*\$ ?(.*)\n\s*\|\s*(OK|ERROR\s*(.*))\n', header)
if m is None:
    print(f"{source}: cannot parse the BEGIN_MBB_TESTS header")
    sys.exit(1)
(cmd, expect, detail) = (m.group(1), 'ERROR' if m.group(3) is not None else 'OK', m.group(3) or 'OK')

if not os.path.exists("cluster.xml"):
    with open('cluster.xml', 'w') as outfile:
        outfile.write("<?xml version='1.0'?>\n")
        outfile.write("<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">\n")
        outfile.write('<platform version="4.1">\n')
        outfile.write(' <cluster id="acme" prefix="node-" radical="0-99" suffix="" speed="1Gf" bw="125MBps" lat="50us"/>\n')
        outfile.write('</platform>\n')

cmd = cmd.replace("mpirun", f"{build_dir}/smpi_script/bin/smpirun -wrapper '{build_dir}/bin/simgrid-mc "
                  "--cfg=model-check/reduction:odpor --log=mc_safety.t:info' -platform ./cluster.xml -analyze "
                  "--cfg=smpi/barrier-finalization:on --cfg=smpi/list-leaks:10 --cfg=model-check/max-depth:10000")
cmd = cmd.replace('${EXE}', binary)
print(f"$ {cmd}", flush=True)
# Run in a new session, to kill the whole process tree on timeout and not only the shell
proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
                        start_new_session=True)
try:
    output = proc.communicate(timeout=300)[0]
    # Same classification order as MBI's simgrid.py: the first matching pattern wins
    outcome = 'other'
    for (pattern, result) in [('MC is currently not supported here', 'failure'),
                              ('Collective communication mismatch', 'Collective mismatch'),
                              ('DEADLOCK DETECTED', 'deadlock'), ('returned MPI_ERR', 'mpierr'),
                              ('Not yet implemented', 'UNIMPLEMENTED'), ('CRASH IN THE PROGRAM', 'failure'),
                              # MBB never frees its plain buffers, so only leaked MPI handles are errors
                              (r'Probable memory leaks in your code: SMPI detected \d+ unfreed MPI handles', 'resleak'),
                              ('Found a data race', 'concurrency'), ('DFS exploration ended.', 'OK')]:
        if re.search(pattern, output):
            outcome = result
            break
except subprocess.TimeoutExpired:
    os.killpg(proc.pid, signal.SIGKILL)
    output = proc.communicate()[0]
    outcome = 'timeout'

if outcome in ['timeout', 'failure', 'UNIMPLEMENTED', 'other'] or (outcome == 'OK') != (expect == 'OK'):
    # Only show the beginning and the end of the output: the model-checker may print the same reports on each trace
    lines = output.splitlines()
    if len(lines) > 200:
        lines = lines[:100] + [f"[{len(lines) - 200} lines omitted]"] + lines[-100:]
    print("\n".join(lines))
    print("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n")
    print(f"SimGrid gave the wrong result on {binary} ({outcome} instead of {detail}).")
    sys.exit(1)
