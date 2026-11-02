# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how the latency of the communications is taken into account in multicore parallel tasks.
It requires the ptask_L07 model (--cfg=host/model:ptask_L07).

Usage: exec-ptask-multicore-latency.py platform_file --cfg=host/model:ptask_L07 [other parameters]
"""

import sys
from simgrid import Engine, this_actor


def runner():
    e = Engine.instance
    comp = [1e9] * 2
    comm = [0.0] * 4
    # Different hosts.
    hosts_diff = [e.host_by_name("MyHost1"), e.host_by_name("MyHost2")]
    start_time = Engine.clock
    this_actor.parallel_execute(hosts_diff, comp, comm)
    this_actor.info(f"Computed 2-core activity on two different hosts. Took {Engine.clock - start_time:g} s")

    # Same host, multicore.
    multicore_host = [e.host_by_name("MyHost1")] * 2
    start_time = Engine.clock
    this_actor.parallel_execute(multicore_host, comp, comm)
    this_actor.info(f"Computed 2-core activity on one 4-core host. Took {Engine.clock - start_time:g} s")

    # Same host, using too many cores
    comp6 = [1e9] * 6
    comm6 = [0.0] * 36
    multicore_overload = [e.host_by_name("MyHost1")] * 6
    start_time = Engine.clock
    this_actor.parallel_execute(multicore_overload, comp6, comm6)
    this_actor.info(f"Computed 6-core activity on a 4-core host. Took {Engine.clock - start_time:g} s")

    # Same host, adding some communication
    comm2 = [0, 1E7, 1E7, 0]
    start_time = Engine.clock
    this_actor.parallel_execute(multicore_host, comp, comm2)
    this_actor.info("Computed 2-core activity on a 4-core host with some communication. "
                    f"Took {Engine.clock - start_time:g} s")

    # See if the multicore execution continues to work after changing pstate
    this_actor.info("Switching machine multicore to pstate 1.")
    e.host_by_name("MyHost1").pstate = 1
    this_actor.info("Switching back to pstate 0.")
    e.host_by_name("MyHost1").pstate = 0

    start_time = Engine.clock
    this_actor.parallel_execute(multicore_host, comp, comm)
    this_actor.info(f"Computed 2-core activity on one 4-core host. Took {Engine.clock - start_time:g} s")

    start_time = Engine.clock
    this_actor.parallel_execute(hosts_diff, comp, comm)
    this_actor.info(f"Computed 2-core activity on two different hosts. Took {Engine.clock - start_time:g} s")


if __name__ == '__main__':
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file [other parameters]")
    e.load_platform(sys.argv[1])

    e.host_by_name("MyHost1").add_actor("test", runner)

    e.run()
    this_actor.info("Simulation done.")
