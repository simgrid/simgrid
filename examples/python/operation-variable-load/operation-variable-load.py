# Copyright (c) 2006-2023. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example demonstrates how to create a variable load for operations.
We consider the following graph:

comm -> exec

With a small load each comm operation is followed by an exec operation.
With a heavy load there is a burst of comm before the exec operation can even finish once.
"""

from argparse import ArgumentParser
import sys
from simgrid import Engine, Operation, CommOp, ExecOp, Actor, this_actor

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

def variable_load(op):
    print('--- Small load ---')
    for i in range(3):
        op.enqueue_execs(1)
        this_actor.sleep_for(100)
    this_actor.sleep_for(1000)
    print('--- Heavy load ---')
    for i in range(3):
        op.enqueue_execs(1)
        this_actor.sleep_for(1)

if __name__ == '__main__':
    args = parse()
    e = Engine(sys.argv)
    e.load_platform(args.platform)
    Operation.init()

    # Retrieve hosts
    tremblay = e.host_by_name('Tremblay')
    jupiter = e.host_by_name('Jupiter')

    # Create operations
    comm = CommOp.init("comm", 1e7, tremblay, jupiter)
    exec = ExecOp.init("exec", 1e9, jupiter)

    # Create the graph by defining dependencies between operations
    comm.add_successor(exec)

    # Add a function to be called when operations end for log purpose
    Operation.on_end_cb(callback)

    # Create the actor that will inject load during the simulation
    Actor.create("input", tremblay, variable_load, comm)

    # runs the simulation
    e.run()
