# Copyright (c) 2006-2023. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example demonstrates basic use of the task plugin.
We model the following graph:

exec1 -> comm -> exec2

exec1 and exec2 are execution tasks.
comm is a communication task.
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

if __name__ == '__main__':
    args = parse()
    e = Engine(sys.argv)
    e.load_platform(args.platform)

    # Retrieve hosts
    tremblay = e.host_by_name('Tremblay')
    jupiter = e.host_by_name('Jupiter')

    # Create tasks
    exec1 = ExecTask.init("exec1", 1e9, tremblay)
    exec2 = ExecTask.init("exec2", 1e9, jupiter)
    comm = CommTask.init("comm", 1e7, tremblay, jupiter)

    # Create the graph by defining dependencies between tasks
    exec1.add_successor(comm)
    comm.add_successor(exec2)

    # Add a function to be called when tasks end for log purpose
    Task.on_completion_cb(callback)

    # Enqueue two firings for task exec1
    exec1.enqueue_firings(2)

    # runs the simulation
    e.run()
