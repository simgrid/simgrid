#! /usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright (c) 2019-2025. The SimGrid Team. All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
Search for symbols documented in both the XML files produced by Doxygen and the python modules,
but not documented with breathe in the RST files.

This script is tailored to SimGrid own needs.

If you are missing some dependencies, try:  pip3 install --requirement docs/requirements.txt
"""

import os
import re
import sys
import xml.etree.ElementTree as ET
import inspect

java_xml_files = [
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1Activity_1_1State.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1ActivitySet.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1Activity.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1Actor.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1AssertionError.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1Barrier.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1CallbackActorHost.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1CallbackActor.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1CallbackBoolean.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1CallbackComm.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1CallbackDHostDouble.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1CallbackDisk.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1CallbackDouble.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1CallbackExec.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1CallbackIo.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1CallbackLink.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1CallbackNetzone.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1CallbackVirtualMachine.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1CallbackVoid.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1Comm.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1ConditionVariable.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1Disk_1_1Operation.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1Disk_1_1SharingPolicy.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1Disk.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1Engine.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1Exec.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1ForcefulKillException.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1Host_1_1LoadPlugin.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1HostFailureException.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1Host.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1Io_1_1OpType.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1Io.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1Link_1_1LoadPlugin.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1Link_1_1SharingPolicy.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1LinkInRoute_1_1Direction.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1LinkInRoute.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1Link.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1Mailbox_1_1IprobeKind.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1Mailbox.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1MessageQueue.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1Mess.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1Mutex.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1NativeLib_1_1FileCleaner.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1NativeLib.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1NetworkFailureException.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1NetZone.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1Semaphore.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1SimgridException.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1TimeoutException.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1VirtualMachine_1_1State.xml',
    'build/xml/classorg_1_1simgrid_1_1s4u_1_1VirtualMachine.xml',
]
cpp_xml_files = [
    # C++ classes
    'build/xml/classsimgrid_1_1s4u_1_1Activity.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Actor.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Barrier.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Comm.xml',
    'build/xml/classsimgrid_1_1s4u_1_1ConditionVariable.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Disk.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Engine.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Exec.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Host.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Io.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Link.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Mailbox.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Mutex.xml',
    'build/xml/classsimgrid_1_1s4u_1_1NetZone.xml',
    'build/xml/classsimgrid_1_1s4u_1_1Semaphore.xml',
    'build/xml/classsimgrid_1_1s4u_1_1VirtualMachine.xml',
    'build/xml/classsimgrid_1_1xbt_1_1signal_3_01R_07P_8_8_8_08_4.xml',
    'build/xml/namespacesimgrid_1_1s4u_1_1this__actor.xml',
    'build/xml/actor_8h.xml',
    'build/xml/barrier_8h.xml',
    'build/xml/cond_8h.xml',
    'build/xml/engine_8h.xml',
    'build/xml/forward_8h.xml',
    'build/xml/host_8h.xml',
    'build/xml/link_8h.xml',
    'build/xml/mailbox_8h.xml',
    'build/xml/mutex_8h.xml',
    'build/xml/semaphore_8h.xml',
    'build/xml/vm_8h.xml',
    'build/xml/zone_8h.xml'
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
        if kind not in python_decl:
            python_decl[kind] = []
        python_decl[kind].append(obj)


    if fullname in python_ignore:
        print("Ignore Python symbol '{}' as requested.".format(fullname))
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
    elif isinstance(elm, (int, str)): # We do have such a data, directly in the SimGrid top module
        found_decl("data", fullname)
#        print('.. autodata:: {}'.format(fullname))
    elif inspect.isclass(type(elm)): # Enum classes are of that kind
        found_decl("data", fullname)
        #print('.. autodata:: {}'.format(fullname))
    elif inspect.ismodule(elm) or inspect.isclass(elm):
        for name, data in inspect.getmembers(elm):
            if name.startswith('__'):
                continue
#            print("Recurse on {}.{}".format(fullname, name))
            handle_python_module("{}.{}".format(fullname, name), elm, data)
    else:
        print('UNHANDLED TYPE {} : {!r} Type: {} Englobing: {} str: {} Members: \n{}\n'.format(fullname, elm, type(elm), englobing, str(elm), inspect.getmembers(elm)))

# Start the recursion on the provided Python modules
for name in python_modules:
    try:
        module = __import__(name)
    except Exception:
        if os.path.exists("../lib") and "../lib" not in sys.path:
            print("Adding ../lib to PYTHONPATH as {} cannot be imported".format(name))
            sys.path.append("../lib")
            try:
                module = __import__(name)
            except Exception:
                print("Cannot import {}, even with PYTHONPATH=../lib".format(name))
                sys.exit(1)
        else:
            print("Cannot import {}".format(name))
            sys.exit(1)
    for sub in dir(module):
        if sub[0] == '_':
            continue
        handle_python_module("{}.{}".format(name, sub), module, getattr(module, sub))

# Forget about the python declarations that were actually done in the RST
for kind in python_decl:
    with os.popen('grep \'[[:blank:]]*auto{}::\' source/*rst|sed \'s/^.*auto{}:: //\''.format(kind, kind)) as pse:
        for fullname in (l.strip() for l in pse):
            if fullname not in python_decl[kind]:
                print("Warning: {} documented but declaration not found in python.".format(fullname))
            else:
                python_decl[kind].remove(fullname)
# Dump the missing ones
for kind in python_decl:
    for fullname in python_decl[kind]:
        print(" .. auto{}:: {}".format(kind, fullname))

################ And now deal with Doxygen declarations
################

def handle_doxygen(files, is_java):
    doxy_funs = {} # {classname: {func_name: [args]} }
    doxy_vars = {} # {classname: [names]}
    doxy_type = {} # {classname: [names]}
    function_marker = "javamethod"   if is_java else "doxygenfunction"
    variable_marker = "javavariable" if is_java else "doxygenvariable"
    separator = "." if is_java else "::"

    # find the declarations in the XML files
    for arg in files:
        if arg[-4:] != '.xml':
            print("Argument '{}' does not end with '.xml'".format(arg))
            continue
        #print("Parse file {}".format(arg))
        tree = ET.parse(arg)
        for elem in tree.findall(".//compounddef"):
            if elem.attrib["kind"] == "class":
                if elem.attrib["prot"] != "public":
                    continue
                if "compoundname" in elem:
                    raise Exception("Compound {} has no 'compoundname' child tag.".format(elem))
                compoundname = elem.find("compoundname").text
                if is_java:
                    compoundname = compoundname.replace('::', '.')
                #print("compoundname {}".format(compoundname))
            elif elem.attrib["kind"] == "file":
                compoundname = ""
            elif elem.attrib["kind"] == "namespace":
                compoundname = elem.find("compoundname").text
                if is_java:
                    compoundname = compoundname.replace('::', '.')
            else:
                print("Element {} is of kind {}".format(elem.attrib["id"], elem.attrib["kind"]))

            for member in elem.findall('.//memberdef'):
                if member.attrib["prot"] != "public":
                    continue
                kind = member.attrib["kind"]
                name = member.find("name").text
                #print("kind:{} compoundname:{} name:{}".format( kind,compoundname, name))
                if kind == "variable":
                    if compoundname not in doxy_vars:
                        doxy_vars[compoundname] = []
                    doxy_vars[compoundname].append(name)
                elif kind == "function":
                    args = member.find('argsstring').text
                    args = re.sub(r'\)[^)]*$', ')', args) # ignore what's after the parameters (eg, '=0' or ' const')

                    if compoundname not in doxy_funs:
                        doxy_funs[compoundname] = {}
                    if name not in doxy_funs[compoundname]:
                        doxy_funs[compoundname][name] = []
                    doxy_funs[compoundname][name].append(args)
                elif kind == "typedef":
                    if compoundname not in doxy_type:
                        doxy_type[compoundname] = []
                    doxy_type[compoundname].append(name)
                elif kind == "friend":
                    pass # Ignore friendship
                else:
                    print(f"member {compoundname}{separator}{name} is of kind {kind}")

    # Forget about the declarations that are done in the RST or the find-missing.ignore file
    with os.popen(f'grep {function_marker}:: find-missing.ignore source/*rst|sed \'s/^.*{function_marker}:: //\'|sed \'s/ *const//\'') as pse:
        for line in (l.strip() for l in pse):
            (klass, obj, args) = (None, None, None)
            if "(" in line:
                (line, args) = line.split('(', 1)
                args = "({}".format(args)
            if separator in line:
                (klass, obj) = line.rsplit(separator, 1)
            else:
                (klass, obj) = ("", line)

            if klass not in doxy_funs:
                print("Warning: {} documented, but class {} not found in doxygen.".format(line, klass))
                continue
            if obj not in doxy_funs[klass]:
                print("Warning: Object '{}' documented but not found in '{}'".format(line, klass))
    #            for obj in doxy_funs[klass]:
    #                print("  found: {}::{}".format(klass, obj))
            elif len(doxy_funs[klass][obj]) == 1:
                del doxy_funs[klass][obj]
            elif args not in doxy_funs[klass][obj]:
                print("Warning: Function {}{} not found in {}".format(obj, args, klass))
            else:
                #print("Found {} in {}".format(line, klass))
                doxy_funs[klass][obj].remove(args)
                if len(doxy_funs[klass][obj]) == 0:
                    del doxy_funs[klass][obj]

    with os.popen(f'grep {variable_marker}:: find-missing.ignore source/*rst|sed \'s/^.*{variable_marker}:: //\'') as pse:
        for line in (l.strip() for l in pse):
            (klass, var) = line.rsplit(separator, 1)

            if klass not in doxy_vars:
                print(f"Warning: {line} documented, but class {klass} not found in doxygen.")
                continue
            if var not in doxy_vars[klass]:
                print(f"Warning: Object {line} documented but not found in '{klass}'")
            else:
    #            print("Found {} in {}".format(line, klass))
                doxy_vars[klass].remove(var)
                if len(doxy_vars[klass]) == 0:
                    del doxy_vars[klass]
    with os.popen('grep doxygentypedef:: find-missing.ignore source/*rst|sed \'s/^.*doxygentypedef:: //\'') as pse:
        for line in (l.strip() for l in pse):
            if separator in line:
                (klass, typ) = line.rsplit(separator, 1)
            else:
                (klass, typ) = ('', line)

            if klass not in doxy_type:
                print("Warning: {} documented, but class {} not found in doxygen.".format(line, klass))
                continue
            if typ not in doxy_type[klass]:
                print("Warning: Type {} documented but not found in '{}'".format(line, klass))
            else:
    #            print("Found {} in {}".format(line, klass))
                doxy_type[klass].remove(typ)
                if len(doxy_type[klass]) == 0:
                    del doxy_type[klass]

    # Dump the undocumented Doxygen declarations
    for obj in sorted(doxy_funs):
        for meth in sorted(doxy_funs[obj]):
            if is_java:
                if obj == '':
                    print(f".. {function_marker}:: {meth}")
                else:
                    print(f".. {function_marker}:: {obj}.{meth}")
            else:
                for args in sorted(doxy_funs[obj][meth]):
                    if obj == '':
                        print(f".. {function_marker}:: {meth}{args}")
                    else:
                        print(f".. {function_marker}:: {obj}::{meth}{args}")

    for obj in doxy_vars:
        for meth in sorted(doxy_vars[obj]):
            print(f".. {variable_marker}:: {obj}{separator}{meth}")

    for obj in doxy_type:
        for meth in sorted(doxy_type[obj]):
            if obj == '':
                print(".. doxygentypedef:: {}".format(meth))
            else:
                print(f".. doxygentypedef:: {obj}{separator}{meth}")


handle_doxygen(cpp_xml_files, False)
handle_doxygen(java_xml_files, True)