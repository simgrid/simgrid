# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how to retrieve the energy consumed by the hosts, and how it is affected by changing the pstate
(DVFS) and by turning hosts off.

Usage: energy-exec.py platform_file [other parameters]
"""

import sys
from simgrid import Engine, Host, sg_host_energy_plugin_init, this_actor


def dvfs():
    host1 = Host.by_name("MyHost1")
    host2 = Host.by_name("MyHost2")

    this_actor.info(f"Energetic profile: {host1.get_property('wattage_per_state')}")
    this_actor.info(f"Initial peak speed={host1.speed:.0E} flop/s; Energy dissipated ={host1.consumed_energy:.0E} J")

    start = Engine.clock
    this_actor.info("Sleep for 10 seconds")
    this_actor.sleep_for(10)
    this_actor.info(f"Done sleeping (duration: {Engine.clock - start:.2f} s). Current peak speed={host1.speed:.0E}; "
                    f"Energy dissipated={host1.consumed_energy:.2f} J")

    # Execute something
    start = Engine.clock
    flop_amount = 100E6
    this_actor.info(f"Run a computation of {flop_amount:.0E} flops")
    this_actor.execute(flop_amount)
    this_actor.info(f"Computation done (duration: {Engine.clock - start:.2f} s). "
                    f"Current peak speed={host1.speed:.0E} flop/s; "
                    f"Current consumption: from {host1.wattmin_at(host1.pstate):.0f}W "
                    f"to {host1.wattmax_at(host1.pstate):.0f}W depending on load; "
                    f"Energy dissipated={host1.consumed_energy:.0f} J")

    # ========= Change power peak =========
    pstate = 2
    host1.pstate = pstate
    this_actor.info(f"========= Requesting pstate {pstate} (speed should be of {host1.pstate_speed(pstate):.0E} flop/s "
                    f"and is of {host1.speed:.0E} flop/s)")

    # Run another computation
    start = Engine.clock
    this_actor.info(f"Run a computation of {flop_amount:.0E} flops")
    this_actor.execute(flop_amount)
    this_actor.info(f"Computation done (duration: {Engine.clock - start:.2f} s). "
                    f"Current peak speed={host1.speed:.0E} flop/s; Energy dissipated={host1.consumed_energy:.0f} J")

    start = Engine.clock
    this_actor.info("Sleep for 4 seconds")
    this_actor.sleep_for(4)
    this_actor.info(f"Done sleeping (duration: {Engine.clock - start:.2f} s). "
                    f"Current peak speed={host1.speed:.0E} flop/s; Energy dissipated={host1.consumed_energy:.0f} J")

    # =========== Turn the other host off ==========
    this_actor.info(f"Turning MyHost2 off, and sleeping another 10 seconds. "
                    f"MyHost2 dissipated {host2.consumed_energy:.0f} J so far.")
    host2.turn_off()
    start = Engine.clock
    this_actor.sleep_for(10)
    this_actor.info(f"Done sleeping (duration: {Engine.clock - start:.2f} s). "
                    f"Current peak speed={host1.speed:.0E} flop/s; Energy dissipated={host1.consumed_energy:.0f} J")


if __name__ == '__main__':
    sg_host_energy_plugin_init()
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file [other parameters]")
    e.load_platform(sys.argv[1])

    e.host_by_name("MyHost1").add_actor("dvfs_test", dvfs)

    e.run()

    this_actor.info("End of simulation.")
