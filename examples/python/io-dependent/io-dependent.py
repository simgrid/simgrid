# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how to declare dependencies between executions and I/O operations.

Usage: io-dependent.py platform_file [other parameters]
"""

import sys
from simgrid import ActivitySet, Engine, Host, IoOpType, this_actor


def test():
    carl = Host.by_name("carl")
    bob_compute = this_actor.exec_init(1e9)
    bob_write = Host.current().disks[0].io_init(4000000, IoOpType.WRITE)
    carl_read = carl.disks[0].io_init(4000000, IoOpType.READ)
    carl_compute = carl.exec_init(1e9)

    pending_activities = ActivitySet([bob_compute, bob_write, carl_read, carl_compute])

    # Name the activities (for logging purposes only)
    bob_compute.name = "bob compute"
    bob_write.name = "bob write"
    carl_read.name = "carl read"
    carl_compute.name = "carl compute"

    # Create the dependencies:
    # 'bob_write' is a successor of 'bob_compute'
    # 'carl_read' is a successor of 'bob_write'
    # 'carl_compute' is a successor of 'carl_read'
    bob_compute.add_successor(bob_write)
    bob_write.add_successor(carl_read)
    carl_read.add_successor(carl_compute)

    # Start the activities.
    bob_compute.start()
    bob_write.start()
    carl_read.start()
    carl_compute.start()

    # Wait for the completion of all activities
    while not pending_activities.empty():
        completed_one = pending_activities.wait_any()
        if completed_one is not None:
            this_actor.info(f"Activity '{completed_one.name}' is complete")


if __name__ == '__main__':
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file [other parameters]")
    e.load_platform(sys.argv[1])

    e.host_by_name("bob").add_actor("bob", test)

    e.run()

    this_actor.info(f"Simulation time {e.clock:g}")
