# Copyright (c) 2006-2023. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example demonstrates how to create a variable load for tasks.
We consider the following graph:

comm -> exec

With a small load each comm task is followed by an exec task.
With a heavy load there is a burst of comm before the exec task can even finish once.
"""

from argparse import ArgumentParser
import sys
from simgrid import Engine, Task, CommTask, ExecTask, Actor, this_actor

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

def variable_load(t):
    print('--- Small load ---')
    for _ in range(3):
        t.enqueue_firings(1)
        this_actor.sleep_for(100)
    this_actor.sleep_for(1000)
    print('--- Heavy load ---')
    for _ in range(3):
        t.enqueue_firings(1)
        this_actor.sleep_for(1)

if __name__ == '__main__':
    args = parse()
    e = Engine(sys.argv)
    e.load_platform(args.platform)

    # Retrieve hosts
    tremblay = e.host_by_name('Tremblay')
    jupiter = e.host_by_name('Jupiter')

    # Create tasks
    comm = CommTask.init("comm", 1e7, tremblay, jupiter)
    exec = ExecTask.init("exec", 1e9, jupiter)

    # Create the graph by defining dependencies between tasks
    comm.add_successor(exec)

    # Add a function to be called when tasks end for log purpose
    Task.on_completion_cb(callback)

    # Create the actor that will inject load during the simulation
    Actor.create("input", tremblay, variable_load, comm)

    # runs the simulation
    e.run()
