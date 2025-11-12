#!/usr/bin/env python3

# Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

import sys
import os
import re
import argparse
import xml.etree.ElementTree as ET

# Factorized script name
SCRIPT_NAME = os.path.basename(sys.argv[0])


def print_usage():
    """Display script usage"""
    print(f"Usage: {SCRIPT_NAME} [OPTIONS] -platform <xmldesc> -hostfile <hostfile> replay-file output-file")


def extract_hosts_from_platform(platform_file):
    """
    Extract host names from a platform XML file using XML parser
    Returns a list of host names
    """
    hosts = []
    try:
        tree = ET.parse(platform_file)
        root = tree.getroot()
        
        # Find all host elements and extract their id attribute
        for host in root.findall(".//*[@id]"):
            if host.tag.endswith('host'):
                hosts.append(host.get('id'))
    except ET.ParseError as e:
        print(f"[{SCRIPT_NAME}] ** error: Could not parse XML file '{platform_file}': {e}")
        sys.exit(1)
    except Exception as e:
        print(f"[{SCRIPT_NAME}] ** error: Failed to extract hosts from '{platform_file}': {e}")
        sys.exit(1)
    
    return hosts


def extract_hosts_from_hostfile(hostfile):
    """
    Process hosts handling host:num_process notations, reading from the provided hostfile
    Returns an expanded list of hosts
    """
    hosts = []
        
    try:
        with open(hostfile, 'r') as f:
            for line in f:
                line = line.strip()
                if line:
                    # Check if the host contains a process count specification
                    match = re.match(r'(.*?):(\d+).*', line)
                    if match:
                        hostname, count = match.groups()
                        hosts.extend([hostname] * int(count))
                    else:
                        hosts.append(line)

    except Exception as e:
        print(f"[{SCRIPT_NAME}] ** error: Failed to read hostfile '{hostfile}': {e}")
        sys.exit(1)
        
    return hosts


def main():
    # Parse arguments
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('-platform', type=str, help='Platform XML description file')
    parser.add_argument('-hostfile', '-machinefile', dest='hostfile', type=str, help='File containing hosts')
    
    # Separate known and unknown arguments
    args, remaining = parser.parse_known_args()
    
    # Extract SimGrid options
    sim_opts = []
    proc_args = []
    
    i = 0
    while i < len(remaining):
        arg = remaining[i]
        if arg.startswith('--cfg=') or arg.startswith('--log='):
            opt_type, opt_value = arg.split('=', 1)
            for opt in opt_value.split(','):
                sim_opts.append(f"{opt_type}={opt}")
        else:
            proc_args.append(arg)
        i += 1
    
    # Check files
    if args.platform and not os.path.isfile(args.platform):
        print(f"[{SCRIPT_NAME}] ** error: the file '{args.platform}' does not exist. Aborting.")
        sys.exit(1)
        
    if args.hostfile and not os.path.isfile(args.hostfile):
        print(f"[{SCRIPT_NAME}] ** error: the file '{args.hostfile}' does not exist. Aborting.")
        sys.exit(1)
    
    if not args.hostfile and not args.platform:
        print("No hostfile nor platform specified.")
        print_usage()
        sys.exit(1)
    
    # Get hosts list
    hosts = []
    if args.hostfile:
        hosts = extract_hosts_from_hostfile(args.hostfile)
    else:
        hosts = extract_hosts_from_platform(args.platform)
    
    # Verify hosts list
    if not hosts:
        if args.hostfile:
            print(f"[{SCRIPT_NAME}] ** error: the hostfile '{args.hostfile}' is empty. Aborting.")
        else:
            print(f"[{SCRIPT_NAME}] ** error: no hosts found in platform file '{args.platform}'. Aborting.")
        sys.exit(1)
    
    # Get application arguments
    if len(proc_args) < 2:
        print("Missing required arguments: replay-file output-file")
        print_usage()
        sys.exit(1)
    
    description_file = proc_args[0]
    application_tmp = proc_args[1]
    
    # Count hosts
    num_hosts = len(hosts)
    
    # Generate application XML
    with open(application_tmp, 'w') as app_file:
        app_file.write('<?xml version=\'1.0\'?>\n')
        app_file.write('<!DOCTYPE platform SYSTEM "https://simgrid.org/simgrid.dtd">\n')
        app_file.write('<platform version="4.1">\n')
        
        if os.path.isfile(description_file):
            num_procs = 0
            
            with open(description_file, 'r') as desc_file:
                for line in desc_file:
                    line = line.strip()
                    if not line:
                        continue
                    
                    parts = line.split()
                    if len(parts) < 4:
                        print(f"Invalid line format in {description_file}: {line}")
                        continue
                    
                    instance = parts[0]
                    hosttrace_file = parts[1]
                    declared_proc_count = int(parts[2])
                    sleep_time = parts[3]
                    
                    # Read host trace
                    hosttrace = []
                    with open(hosttrace_file, 'r') as trace_file:
                        hosttrace = [line.strip() for line in trace_file if line.strip()]
                    
                    num_procs_mine = len(hosttrace)
                    
                    if num_procs_mine != declared_proc_count:
                        print(f"Declared number of processes for instance {instance}: {declared_proc_count} is not the same as the one in the replay files: {num_procs_mine}. Please check consistency of these information")
                        sys.exit(1)
                    
                    for i in range(num_procs_mine):
                        j = num_procs % num_hosts
                        host = hosts[j]
                        
                        app_file.write(f'  <actor host="{host}" function="{instance}"> <!-- function name used only for logging -->\n')
                        app_file.write(f'    <argument value="{instance}"/> <!-- instance -->\n')
                        app_file.write(f'    <argument value="{i}"/> <!-- rank -->\n')
                        app_file.write(f'    <argument value="{hosttrace[i]}"/>\n')
                        app_file.write(f'    <argument value="{sleep_time}"/> <!-- delay -->\n')
                        app_file.write('  </actor>\n')
                        
                        num_procs += 1
        else:
            print(f"File not found: {description_file}")
            sys.exit(1)
        
        app_file.write('</platform>\n')
    
    return 0


if __name__ == "__main__":
    sys.exit(main())
