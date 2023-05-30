# Copyright (c) 2006-2023. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

from argparse import ArgumentParser
import sys
from simgrid import Engine, Operation, ExecOp, IoOp, IoOpType

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
    bob = e.host_by_name('bob')
    carl = e.host_by_name('carl')

    # Create operations
    exec1 = ExecOp.init("exec1", 1e9, bob)
    exec2 = ExecOp.init("exec2", 1e9, carl)
    write = IoOp.init("write", 1e7, bob.disks[0], IoOpType.WRITE)
    read = IoOp.init("read", 1e7, carl.disks[0], IoOpType.READ)

   # Create the graph by defining dependencies between operations
    exec1.add_successor(write)
    write.add_successor(read)
    read.add_successor(exec2)

    # Add a function to be called when operations end for log purpose
    Operation.on_end_cb(callback)

    # Enqueue two executions for operation exec1
    exec1.enqueue_execs(2)

    # runs the simulation
    e.run()