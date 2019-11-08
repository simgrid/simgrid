#! /usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2019. The SimGrid Team.
# All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
Search for symbols documented in both the XML files produced by Doxygen and the python modules,
but not documented with autodoxy in the RST files.

This script is tailored to SimGrid own needs and should be made more generic for autodoxy.
"""

import fnmatch
import os
import re
import sys
import xml.etree.ElementTree as ET
import inspect

xml_files = [
#    'build/xml/classsimgrid_1_1s4u_1_1Activity.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Actor.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Barrier.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Comm.xml',
    'build/xml/classsimgrid_1_1s4u_1_1ConditionVariable.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Disk.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Engine.xml',
    'build/xml/classsimgrid_1_1s4u_1_1ExecPar.xml',
    'build/xml/classsimgrid_1_1s4u_1_1ExecSeq.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Exec.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Host.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Io.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Link.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Mailbox.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Mutex.xml',
    'build/xml/classsimgrid_1_1s4u_1_1NetZone.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Semaphore.xml',
    'build/xml/classsimgrid_1_1s4u_1_1VirtualMachine.xml'
]

python_modules = [
    'simgrid'
]
python_ignore = [
    'simgrid.ActorKilled'
]


############ Search the python elements in the source, and report undocumented ones
############

# data structure in which we store the declaration we find
python_decl = {}

def handle_python_module(fullname, englobing, elm):
    """Recursive function exploring the python modules."""

    def found_decl(kind, obj):
        """Helper function that add an object in the python_decl data structure"""
        if not kind in python_decl: python_decl[kind] = []
        python_decl[kind].append(obj)


    if fullname in python_ignore:
        print ("Ignore Python symbol '{}' as requested.".format(fullname))
        return

    if inspect.isroutine(elm) and inspect.isclass(englobing):
        found_decl("method", fullname)
#        print('.. automethod:: {}'.format(fullname))
    elif inspect.isroutine(elm) and (not inspect.isclass(englobing)):
        found_decl("function", fullname)
#        print('.. autofunction:: {}'.format(fullname))
    elif inspect.isdatadescriptor(elm):
        found_decl("attribute", fullname)
#        print('.. autoattribute:: {}'.format(fullname))
    elif isinstance(elm, str) or isinstance(elm, int): # We do have such a data, directly in the SimGrid top module
        found_decl("data", fullname)
#        print('.. autodata:: {}'.format(fullname))
    elif inspect.ismodule(elm) or inspect.isclass(elm):
        for name, data in inspect.getmembers(elm):
            if name.startswith('__'):
                continue
#            print("Recurse on {}.{}".format(fullname, name))
            handle_python_module("{}.{}".format(fullname, name), elm, data)
    else:
        print('UNHANDLED TYPE {} : {!r} Type: {}'.format(fullname, elm, type(elm)))

# Start the recursion on the provided Python modules
for name in python_modules:
    module = __import__(name)
    for sub in dir(module):
        if sub[0] == '_':
            continue
        handle_python_module("{}.{}".format(name, sub), module, getattr(module, sub))

# Forget about the python declarations that were actually done in the RST
for kind in python_decl:
    with os.popen('grep \'[[:blank:]]*auto{}::\' source/*rst|sed \'s/^.*auto{}:: //\''.format(kind, kind)) as pse:
        for fullname in (l.strip() for l in pse):
            if not fullname in python_decl[kind]:
                print("Warning: {} documented but declaration not found in python.".format(fullname))
            else:
                python_decl[kind].remove(fullname)
# Dump the missing ones
for kind in python_decl:
    for fullname in python_decl[kind]:
        print("Missing decl: .. auto{}:: {}".format(kind, fullname))

################ And now deal with Doxygen declarations
################

doxy_funs = {} # {classname: {func_name: [args]} }
doxy_vars = {} # {classname: [names]}

# find the declarations in the XML files
for arg in xml_files[:1]:
    if arg[-4:] != '.xml':
        print ("Argument '{}' does not end with '.xml'".format(arg))
        continue
    print("Parse file {}".format(arg))
    tree = ET.parse(arg)
    for elem in tree.findall(".//compounddef"):
        if elem.attrib["prot"] != "public":
            continue
        if "compoundname" in elem:
            raise Exception("Compound {} has no 'compoundname' child tag.".format(elem))
        compoundname = elem.find("compoundname").text
        #print ("compoundname {}".format(compoundname))
        for member in elem.findall('.//memberdef'):
            if member.attrib["prot"] != "public":
                continue
            kind = member.attrib["kind"]
            name = member.find("name").text
            if kind == "variable":
                if not compoundname in doxy_vars: doxy_vars[compoundname] = []
                doxy_vars[compoundname].append(name)
            elif kind == "function":
                args = member.find('argsstring').text
                args = re.sub('\)[^)]*$', ')', args) # ignore what's after the parameters (eg, '=0' or ' const')

                if not compoundname in doxy_funs: doxy_funs[compoundname] = {}
                if not name in doxy_funs[compoundname]: doxy_funs[compoundname][name] = []
                doxy_funs[compoundname][name].append(args)
            else:
                print ("member {}::{} is of kind {}".format(compoundname, name, kind))

# Forget about the declarations that are done in the RST
with os.popen('grep autodoxymethod:: source/*rst|sed \'s/^.*autodoxymethod:: //\'') as pse:
    for line in (l.strip() for l in pse):
        (klass, obj, args) = (None, None, None)
        if "(" in line:
            (line, args) = line.split('(', 1)
            args = "({}".format(args)
        (klass, obj) = line.rsplit('::', 1)

        if not klass in doxy_funs:
            print("Warning: {} documented, but class {} not found in doxygen.".format(line, klass))
            continue
        if not obj in doxy_funs[klass]:
            print("Warning: Object {} documented but not found in {}".format(line, klass))
        elif len(doxy_funs[klass][obj])==1:
            del doxy_funs[klass][obj]
        elif not args in doxy_funs[klass][obj]:
            print("Warning: Function {}{} not found in {}".format(obj, args, klass))
        else:
#            print("Found {} in {}".format(line, klass))
            doxy_funs[klass][obj].remove(args)
            if len(doxy_funs[klass][obj]) == 0:
                del doxy_funs[klass][obj]

# Dump the undocumented Doxygen declarations 
for obj in doxy_funs:
    for meth in doxy_funs[obj]:
        for args in doxy_funs[obj][meth]:
            print("Missing decl: .. autodoxymethod:: {}::{}{}".format(obj, meth, args))

for obj in doxy_vars:
    for meth in doxy_vars[obj]:
        print("Missing decl: .. autodoxyfield:: {}::{}".format(obj, meth))

