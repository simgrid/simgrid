#! /usr/bin/env python3

# Copyright 2021-2022. The SimGrid Team. All rights reserved.
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

# This script runs a previously compiled MBI script and checks on the result.
# This file was written for the SimGrid project. Parts coming from the MBI projets are MBIutils.py and simgrid.py

import sys
import os

import MBIutils as mbi
import simgrid as sg

if len(sys.argv) != 4:
    print(f"Usage: MBI.py binary source (received: {sys.argv})")
    sys.exit(1)

if not os.path.exists("cluster.xml"):
    with open('cluster.xml', 'w') as outfile:
        outfile.write("<?xml version='1.0'?>\n")
        outfile.write("<!DOCTYPE platform SYSTEM \"https://simgrid.org/simgrid.dtd\">\n")
        outfile.write('<platform version="4.1">\n')
        outfile.write(' <cluster id="acme" prefix="node-" radical="0-99" suffix="" speed="1Gf" bw="125MBps" lat="50us"/>\n')
        outfile.write('</platform>\n')


simgrid = sg.Tool()

(name, path, binary, filename) = sys.argv
for test in mbi.parse_one_code(filename):
    execcmd = test['cmd'].replace("mpirun", f"{path}/smpi_script/bin/smpirun -wrapper '{path}/bin/simgrid-mc --log=mc_safety.t:info' -platform ./cluster.xml -analyze --cfg=smpi/finalization-barrier:on --cfg=smpi/list-leaks:10 --cfg=model-check/max-depth:10000")
    execcmd = execcmd.replace('${EXE}', binary)
    execcmd = execcmd.replace('$zero_buffer', "--cfg=smpi/buffering:zero")
    execcmd = execcmd.replace('$infty_buffer', "--cfg=smpi/buffering:infty")

    if os.path.exists(f'{filename}.txt'):
        os.remove(f'{filename}.txt')
    mbi.run_cmd(buildcmd="", execcmd=execcmd, cachefile=filename, filename=filename, binary=binary, timeout=300, batchinfo="", read_line_lambda=None)
    outcome = simgrid.parse(filename)

    (res_category, elapsed, diagnostic, outcome) = mbi.categorize(simgrid, "simgrid", filename, test['expect'])

    if res_category != "TRUE_NEG" and res_category != "TRUE_POS":
        print("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n")
        print(f"SimGrid gave the wrong result ({outcome} instead of {test['expect']}).")
        sys.exit(1)
