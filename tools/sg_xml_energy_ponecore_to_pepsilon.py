#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2019. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

'''Update 3-part energy consumption syntax in SimGrid XML platform files.

- watt_per_state: "pIdle:pOneCore:pFull" -> "pIdle:pEpsilon:pFull"
  This is done by computing pEpsilon from pOneCore, pFull and #core.'''
import fnmatch
import os
import sys
import xml.etree.ElementTree as ET

class TreeBuilderWithComments(ET.TreeBuilder):
    def comment(self, data):
        self.start(ET.Comment, {})
        self.data(data)
        self.end(ET.Comment)

def update_platform_file(filename):
    comment_tb = TreeBuilderWithComments()
    tree = ET.parse(filename, parser=ET.XMLParser(target=comment_tb))
    root = tree.getroot()

    parent_dict = {c:p for p in root.iter() for c in p}

    for prop in root.iter('prop'):
        if 'id' in prop.attrib and prop.attrib['id'] == 'watt_per_state':
            # Parse energy consumption and core count
            values_str = "".join(prop.attrib['value'].split()) # remove whitespaces
            values = values_str.split(',')
            nb_core = 1
            if 'core' in parent_dict[prop].attrib:
                nb_core = int(parent_dict[prop].attrib['core'])
            if nb_core < 1: raise Exception(f'Invalid core count: {nb_core}')

            # If a middle value is given, pIdle:pOneCore:pFull is assumed
            # and converted to pIdle:pEpsilon:pFull
            consumption_per_pstate = []
            update_required = False
            for value in values:
                powers = value.split(':')
                if len(powers) == 3:
                    update_required = True
                    (pIdle, p1, pFull) = [float(x) for x in powers]
                    if nb_core == 1:
                        if p1 != pFull:
                            raise Exception('Invalid energy consumption: ' +
                                "A 1-core host has pOneCore != pFull " +
                                f'({p1} != {pFull}).\n' +
                                'Original watt_per_state value: "{}"'.format(prop.attrib['value']))
                        pEpsilon = pFull
                    else:
                        pEpsilon = p1 - ((pFull - p1) / (nb_core - 1))
                    consumption_per_pstate.append(f"{pIdle}:{pEpsilon}:{pFull}")
                    if pIdle > pEpsilon:
                        print(f"WARNING: pIdle > pEpsilon ({pIdle} > {pEpsilon})")
                else: # len(powers) == 2
                    if nb_core == 1:
                        update_required = True
                        (pIdle, pFull) = [float(x) for x in powers]
                        pEpsilon = pFull
                        consumption_per_pstate.append(f"{pIdle}:{pEpsilon}:{pFull}")
                        print(f"WARNING: putting {pFull} as pEpsilon by default for a single core")
                    else:
                        consumption_per_pstate.append(value)

            if update_required:
                updated_value = ', '.join(consumption_per_pstate)
                print(f'"{values_str}" -> "{updated_value}" (core={nb_core})')
                prop.attrib['value'] = updated_value

    with open(filename, 'w', encoding='utf-8') as output_file:
        # xml.etree.ElementTree does not handle doctypes =/
        # https://stackoverflow.com/questions/15304229/convert-python-elementtree-to-string
        content = '''<?xml version='1.0' encoding='utf-8'?>
<!DOCTYPE platform SYSTEM "https://simgrid.org/simgrid.dtd">
{}
'''.format(ET.tostring(root, encoding="unicode"))
        output_file.write(content)

if __name__ == '__main__':
    usage = "usage: {cmd} FILE\n\n{doc}".format(cmd=sys.argv[0], doc=__doc__)

    if len(sys.argv) != 2:
        print(usage)
        sys.exit(1)

    update_platform_file(sys.argv[1])
