# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how to simulate variability for CPUs, using multiplicative factors.

Usage: exec-cpu-factors.py [other parameters]
"""

import sys
from simgrid import Engine, this_actor


def runner():
    computation_amount = this_actor.get_host().speed

    this_actor.info(f"Executing 1 tasks of {computation_amount:g} flops, should take 1 second.")
    this_actor.execute(computation_amount)
    this_actor.info(f"Executing 1 tasks of {computation_amount / 1000:g} flops, it would take .001s without factor. "
                    "It'll take .002s")
    this_actor.execute(computation_amount / 1000)

    this_actor.info("Finished executing. Goodbye now!")


def cpu_variability(host, flops):
    """Creates variability for tasks smaller than 1% of CPU power. Unrealistic, for learning purposes."""
    factor = 0.5 if flops < host.speed / 100 else 1.0
    this_actor.info(f"Host {host.name}, task with {flops:f} flops, new factor {factor:f}")
    return factor


def load_platform(e):
    """Create a simple 1-host platform"""
    zone = e.netzone_root
    runner_host = zone.add_host("runner", 1e6)
    runner_host.set_cpu_factor_cb(cpu_variability)
    zone.seal()

    # create actor runner
    runner_host.add_actor("runner", runner)


if __name__ == '__main__':
    e = Engine(sys.argv)

    # create platform
    load_platform(e)

    # runs the simulation
    e.run()
