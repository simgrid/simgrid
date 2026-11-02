# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how to run multi-threaded executions, that leverage several cores of the same host.

Usage: exec-threads.py platform_file [other parameters]
"""

import sys
from simgrid import ActivitySet, Engine, this_actor


def runner():
    e = Engine.instance
    multicore_host = e.host_by_name("MyHost1")
    # First test with less than, same number as, and more threads than cores
    start_time = Engine.clock
    this_actor.thread_execute(multicore_host, 1e9, 2)
    this_actor.info(f"Computed 2-thread activity on a 4-core host. Took {Engine.clock - start_time:g} s")

    start_time = Engine.clock
    this_actor.thread_execute(multicore_host, 1e9, 4)
    this_actor.info(f"Computed 4-thread activity on a 4-core host. Took {Engine.clock - start_time:g} s")

    start_time = Engine.clock
    this_actor.thread_execute(multicore_host, 1e9, 6)
    this_actor.info(f"Computed 6-thread activity on a 4-core host. Took {Engine.clock - start_time:g} s")

    background_task = this_actor.exec_async(2.5e9)
    this_actor.info("Start a 1-thread background task on the 4-core host.")

    start_time = Engine.clock
    this_actor.thread_execute(multicore_host, 1e9, 2)
    this_actor.info(f"Computed 2-thread activity on a 4-core host. Took {Engine.clock - start_time:g} s")

    start_time = Engine.clock
    this_actor.thread_execute(multicore_host, 1e9, 4)
    this_actor.info(f"Computed 4-thread activity on a 4-core host. Took {Engine.clock - start_time:g} s")

    background_task.wait()
    this_actor.info("The background task has ended.")

    background_task = this_actor.thread_execute_async(2e9, 4)
    this_actor.info("Start a 4-thread background task on the 4-core host.")

    this_actor.info("Sleep for 5 seconds before starting another competing task")
    this_actor.sleep_for(5)

    start_time = Engine.clock
    this_actor.execute(1e9)
    this_actor.info(f"Computed 1-thread activity on a 4-core host. Took {Engine.clock - start_time:g} s")

    background_task.wait()
    this_actor.info("The background task has ended.")

    start_time = Engine.clock
    this_actor.info("Asynchronously start two 2-thread tasks on the 4-core host.")
    task1 = this_actor.thread_execute_async(1e9, 2)
    task2 = this_actor.thread_execute_async(1e9, 2)
    pending = ActivitySet([task1, task2])

    while not pending.empty():
        completed_one = pending.wait_any()
        if completed_one is not None:
            this_actor.info("Activity completed")
    this_actor.info(f"Computed two 2-thread activities on a 4-core host. Took {Engine.clock - start_time:g} s")


if __name__ == '__main__':
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file [other parameters]")
    e.load_platform(sys.argv[1])

    e.host_by_name("MyHost1").add_actor("test", runner)

    e.run()
    this_actor.info("Simulation done.")
