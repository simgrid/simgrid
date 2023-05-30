# Copyright (c) 2006-2023. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
/* This example demonstrates how to dynamically modify a graph of operations.
 *
 * Assuming we have two instances of a service placed on different hosts,
 * we want to send data alternatively to thoses instances.
 *
 * We consider the following graph:

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
 */
 """

from argparse import ArgumentParser
import sys
from simgrid import Engine, Operation, CommOp, ExecOp

def parse():
    parser = ArgumentParser()
    parser.add_argument(
        '--platform',
        type=str,
        required=True,
        help='path to the platform description'
    )
    return parser.parse_args()

def callback(op):
    print(f'[{Engine.clock}] Operation {op} finished ({op.count})')

def switch(op, hosts, execs):
    comm0.destination = hosts[op.count % 2]
    comm0.remove_successor(execs[op.count % 2 - 1])
    comm0.add_successor(execs[op.count % 2])

if __name__ == '__main__':
    args = parse()
    e = Engine(sys.argv)
    e.load_platform(args.platform)
    Operation.init()

    # Retrieve hosts
    tremblay = e.host_by_name('Tremblay')
    jupiter = e.host_by_name('Jupiter')
    fafard = e.host_by_name('Fafard')

    # Create operations
    comm0 = CommOp.init("comm0")
    comm0.bytes = 1e7
    comm0.source = tremblay
    exec1 = ExecOp.init("exec1", 1e9, jupiter)
    exec2 = ExecOp.init("exec2", 1e9, fafard)
    comm1 = CommOp.init("comm1", 1e7, jupiter, tremblay)
    comm2 = CommOp.init("comm2", 1e7, fafard, tremblay)

    # Create the initial graph by defining dependencies between operations
    exec1.add_successor(comm1)
    exec2.add_successor(comm2)

    # Add a function to be called when operations end for log purpose
    Operation.on_end_cb(callback)

    # Add a function to be called before each executions of comm0
    # This function modifies the graph of operations by adding or removing
    # successors to comm0
    comm0.on_this_start(lambda op: switch(op, [jupiter, fafard], [exec1,exec2]))

    # Enqueue two executions for operation exec1
    comm0.enqueue_execs(4)

    # runs the simulation
    e.run()
