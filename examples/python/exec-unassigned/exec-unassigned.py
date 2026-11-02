# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows that an execution does not actually start until it is assigned to a host.

Usage: exec-unassigned.py platform_file [other parameters]
"""

import sys
from simgrid import Engine, Exec, Host, this_actor


def worker():
    # Define an amount of work that should take 1 second to execute.
    computation_amount = this_actor.get_host().speed

    # Create an unassigned activity and start it. It will not actually start, because it's not assigned to any host yet
    activity = Exec.init().set_flops_amount(computation_amount)
    activity.name = "exec"
    activity.start()

    # Wait for a while
    this_actor.sleep_for(10)

    # Assign the activity to the current host. This triggers its start, then wait for its completion.
    activity.host = Host.current()
    activity.wait()
    this_actor.info(f"Exec '{activity.name}' is complete")


if __name__ == '__main__':
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file [other parameters]")
    e.load_platform(sys.argv[1])

    e.host_by_name("Fafard").add_actor("worker", worker)

    e.run()

    this_actor.info(f"Simulation time {e.clock:g}")
