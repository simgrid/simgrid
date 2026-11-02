# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how to wait for an execution with a timeout. A TimeoutException is raised if the execution is not
over when the timeout expires.

Usage: exec-waitfor.py platform_file [other parameters]
"""

import sys
from simgrid import Engine, TimeoutException, this_actor


def worker():
    amount = 5 * this_actor.get_host().speed
    this_actor.info("Create an activity that should run for 5 seconds")
    activity = this_actor.exec_async(amount)

    # Now that execution is started, wait for 3 seconds.
    this_actor.info("But let it end after 3 seconds")
    try:
        activity.wait_for(3)
        this_actor.info("Execution complete")
    except TimeoutException:
        this_actor.info("Execution Wait Timeout!")

    # Do it again, but this time with a timeout greater than the duration of the execution
    this_actor.info("Create another activity that should run for 5 seconds and wait for it for 6 seconds")
    activity = this_actor.exec_async(amount)
    try:
        activity.wait_for(6)
        this_actor.info("Execution complete")
    except TimeoutException:
        this_actor.info("Execution Wait Timeout!")

    this_actor.info("Finally test with a parallel execution")
    hosts = Engine.instance.all_hosts
    hosts_count = len(hosts)
    computation_amounts = [1e9] * hosts_count  # 1 Gflop on each host
    communication_amounts = [0] * (hosts_count * hosts_count)
    for i in range(hosts_count):
        for j in range(i + 1, hosts_count):
            communication_amounts[i * hosts_count + j] = 1e7  # 10 MB

    activity = this_actor.exec_init(hosts, computation_amounts, communication_amounts)
    try:
        activity.wait_for(2)
        this_actor.info("Parallel Execution complete")
    except TimeoutException:
        this_actor.info("Parallel Execution Wait Timeout!")


if __name__ == '__main__':
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file [other parameters]")
    e.load_platform(sys.argv[1])

    e.host_by_name("Tremblay").add_actor("worker", worker)

    e.run()
