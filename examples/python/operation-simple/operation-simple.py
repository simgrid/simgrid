# Copyright (c) 2006-2023. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example demonstrates basic use of the operation plugin.
We model the following graph:

exec1 -> comm -> exec2

exec1 and exec2 are execution operations.
comm is a communication operation.
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

if __name__ == '__main__':
    args = parse()
    e = Engine(sys.argv)
    e.load_platform(args.platform)
    Operation.init()

    # Retrieve hosts
    tremblay = e.host_by_name('Tremblay')
    jupiter = e.host_by_name('Jupiter')

    # Create operations
    exec1 = ExecOp.init("exec1", 1e9, tremblay)
    exec2 = ExecOp.init("exec2", 1e9, jupiter)
    comm = CommOp.init("comm", 1e7, tremblay, jupiter)

    # Create the graph by defining dependencies between operations
    exec1.add_successor(comm)
    comm.add_successor(exec2)

    # Add a function to be called when operations end for log purpose
    Operation.on_end_cb(callback)

    # Enqueue two executions for operation exec1
    exec1.enqueue_execs(2)

    # runs the simulation
    e.run()

