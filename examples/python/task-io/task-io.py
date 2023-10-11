# Copyright (c) 2006-2023. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

from argparse import ArgumentParser
import sys
from simgrid import Engine, Task, ExecTask, IoTask, IoOpType

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
    bob = e.host_by_name('bob')
    carl = e.host_by_name('carl')

    # Create tasks
    exec1 = ExecTask.init("exec1", 1e9, bob)
    exec2 = ExecTask.init("exec2", 1e9, carl)
    write = IoTask.init("write", 1e7, bob.disks[0], IoOpType.WRITE)
    read = IoTask.init("read", 1e7, carl.disks[0], IoOpType.READ)

   # Create the graph by defining dependencies between tasks
    exec1.add_successor(write)
    write.add_successor(read)
    read.add_successor(exec2)

    # Add a function to be called when tasks end for log purpose
    Task.on_completion_cb(callback)

    # Enqueue two firings for task exec1
    exec1.enqueue_firings(2)

    # runs the simulation
    e.run()
