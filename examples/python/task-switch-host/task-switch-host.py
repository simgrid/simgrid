# Copyright (c) 2006-2023. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example demonstrates how to dynamically modify a graph of tasks.

Assuming we have two instances of a service placed on different hosts,
we want to send data alternatively to thoses instances.

We consider the following graph:

           comm1
     ┌────────────────────────┐
     │                        │
     │               Fafard   │
     │              ┌───────┐ │
     │      ┌──────►│ exec1 ├─┘
     ▼      │       └───────┘
 Tremblay ──┤comm0
     ▲      │        Jupiter
     │      │       ┌───────┐
     │      └──────►│ exec2 ├─┐
     │              └───────┘ │
     │                        │
     └────────────────────────┘
           comm2

"""

from argparse import ArgumentParser
import sys
from simgrid import Engine, Task, CommTask, ExecTask

def parse():
    parser = ArgumentParser()
    parser.add_argument(
        '--platform',
        type=str,
        required=True,
        help='path to the platform description'
    )
    return parser.parse_args()

def callback(t):
    print(f'[{Engine.clock}] {t} finished ({t.get_count()})')

def switch_destination(t, hosts):
    t.destination = hosts[switch_destination.count % 2]
    switch_destination.count += 1
switch_destination.count = 0

def switch_successor(t, execs):
    t.remove_successor(execs[t.get_count() % 2])
    t.add_successor(execs[t.get_count() % 2 - 1])

if __name__ == '__main__':
    args = parse()
    e = Engine(sys.argv)
    e.load_platform(args.platform)

    # Retrieve hosts
    tremblay = e.host_by_name('Tremblay')
    jupiter = e.host_by_name('Jupiter')
    fafard = e.host_by_name('Fafard')

    # Create tasks
    comm0 = CommTask.init("comm0")
    comm0.bytes = 1e7
    comm0.source = tremblay
    exec1 = ExecTask.init("exec1", 1e9, jupiter)
    exec2 = ExecTask.init("exec2", 1e9, fafard)
    comm1 = CommTask.init("comm1", 1e7, jupiter, tremblay)
    comm2 = CommTask.init("comm2", 1e7, fafard, tremblay)

    # Create the initial graph by defining dependencies between tasks
    exec1.add_successor(comm1)
    exec2.add_successor(comm2)

    # Add a callback when tasks end for log purpose
    Task.on_completion_cb(callback)

    # Add a callback before each firing of comm0
    # It switches the destination of comm0
    comm0.on_this_start_cb(lambda t: switch_destination(t, [jupiter, fafard]))

    # Add a callback before comm0 send tokens to successors
    # It switches the successor of comm0
    comm0.on_this_completion_cb(lambda t: switch_successor(t, [exec1,exec2]))

    # Enqueue two firings for task exec1
    comm0.enqueue_firings(4)

    # runs the simulation
    e.run()
