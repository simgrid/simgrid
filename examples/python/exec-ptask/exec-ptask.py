# Copyright (c) 2018-2023. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.


# This script does exactly the same thing as file s4u-exec-ptask.cpp

import sys
from simgrid import Actor, Engine, Host, this_actor, TimeoutException

def runner():
    hosts = Engine.instance.all_hosts
    hosts_count = len(hosts)

    # Test 1
    this_actor.info("First, build a classical parallel activity, with 1 Gflop to execute on each node, "
               "and 10MB to exchange between each pair")
    computation_amounts = [1e9]*hosts_count
    communication_amounts = [0]*hosts_count*hosts_count
    for i in range(hosts_count):
        for j in range(i+1, hosts_count):
            communication_amounts[i * hosts_count + j] = 1e7
    this_actor.parallel_execute(hosts, computation_amounts, communication_amounts)

    # Test 2
    this_actor.info("We can do the same with a timeout of 10 seconds enabled.")
    activity = this_actor.exec_init(hosts, computation_amounts, communication_amounts)
    try:
        activity.wait_for(10.0)
        sys.exit("Woops, this did not timeout as expected... Please report that bug.")
    except TimeoutException:
        this_actor.info("Caught the expected timeout exception.")
        activity.cancel()

    # Test 3
    this_actor.info("Then, build a parallel activity involving only computations (of different amounts) and no communication")
    computation_amounts = [3e8, 6e8, 1e9]
    communication_amounts = []
    this_actor.parallel_execute(hosts, computation_amounts, communication_amounts)

    # Test 4
    this_actor.info("Then, build a parallel activity with no computation nor communication (synchro only)")
    computation_amounts = []
    this_actor.parallel_execute(hosts, computation_amounts, communication_amounts)

    # Test 5
    this_actor.info("Then, Monitor the execution of a parallel activity")
    computation_amounts = [1e6]*hosts_count
    communication_amounts = [0, 1e6, 0, 0, 0, 1e6, 1e6, 0, 0]
    activity = this_actor.exec_init(hosts, computation_amounts, communication_amounts)
    activity.start()
    while not activity.test():
        ratio = activity.remaining_ratio * 100
        this_actor.info(f"Remaining flop ratio: {ratio:.0f}%")
        this_actor.sleep_for(5)
    activity.wait()

    # Test 6
    this_actor.info("Finally, simulate a malleable task (a parallel execution that gets reconfigured after its start).")
    this_actor.info("  - Start a regular parallel execution, with both comm and computation")
    computation_amounts = [1e6]*hosts_count
    communication_amounts = [0, 1e6, 0, 0, 1e6, 0, 1e6, 0, 0]
    activity = this_actor.exec_init(hosts, computation_amounts, communication_amounts)
    activity.start()
    this_actor.sleep_for(10)
    remaining_ratio = activity.remaining_ratio
    this_actor.info(f"  - After 10 seconds, {remaining_ratio*100:.2f}% remains to be done. Change it from 3 hosts to 2 hosts only.")
    this_actor.info("    Let's first suspend the task.")
    activity.suspend()
    this_actor.info("  - Now, simulate the reconfiguration (modeled as a comm from the removed host to the remaining ones).")
    rescheduling_comp = [0, 0, 0]
    rescheduling_comm = [0, 0, 0, 0, 0, 0, 25000, 25000, 0]
    this_actor.parallel_execute(hosts, rescheduling_comp, rescheduling_comm)
    this_actor.info("  - Now, let's cancel the old task and create a new task with modified comm and computation vectors:")
    this_actor.info("    What was already done is removed, and the load of the removed host is shared between remaining ones.")
    for i in range(2):
        # remove what we've done so far, for both comm and compute load
        computation_amounts[i] *= remaining_ratio
        communication_amounts[i] *= remaining_ratio
        # The work from 1 must be shared between 2 remaining ones. 1/2=50% of extra work for each
        computation_amounts[i] *= 1.5
        communication_amounts[i] *= 1.5
    hosts = hosts[:2]
    computation_amounts = computation_amounts[:2]
    remaining_comm = communication_amounts[1]
    communication_amounts = [0, remaining_comm, remaining_comm, 0]
    activity.cancel()
    activity = this_actor.exec_init(hosts, computation_amounts, communication_amounts)
    this_actor.info("  - Done, let's wait for the task completion")
    activity.wait()
    this_actor.info("Goodbye now!")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(f"Syntax: {sys.argv[0]} <platform_file>")
    platform = sys.argv[1]
    engine = Engine.instance
    Engine.set_config("host/model:ptask_L07")  # /!\ this is required for running ptasks
    engine.load_platform(platform)
    Actor.create("foo", engine.host_by_name("MyHost1"), runner)
    engine.run()
