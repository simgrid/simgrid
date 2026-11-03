# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how to model the energy consumption of a host during its boot and shutdown, by using specific
pstates that encode these states in the platform.

Usage: energy-boot.py platform_file [other parameters]
"""

import sys
from simgrid import Engine, Host, sg_host_energy_plugin_init, this_actor


def simulate_bootup(host):
    previous_pstate = host.pstate

    this_actor.info("Switch to virtual pstate 3, that encodes the 'booting up' state in that platform")
    host.pstate = 3

    this_actor.info("Actually start the host")
    host.turn_on()

    this_actor.info("Wait 150s to simulate the boot time.")
    this_actor.sleep_for(150)

    this_actor.info(f"The host is now up and running. Switch back to previous pstate {previous_pstate}")
    host.pstate = previous_pstate


def simulate_shutdown(host):
    previous_pstate = host.pstate

    this_actor.info("Switch to virtual pstate 4, that encodes the 'shutting down' state in that platform")
    host.pstate = 4

    this_actor.info("Wait 7 seconds to simulate the shutdown time.")
    this_actor.sleep_for(7)

    this_actor.info(f"Switch back to previous pstate {previous_pstate}, that will be used on reboot.")
    host.pstate = previous_pstate

    this_actor.info("Actually shutdown the host")
    host.turn_off()


def monitor():
    host1 = Host.by_name("MyHost1")

    this_actor.info(f"Initial pstate: {host1.pstate}; Energy dissipated so far:{host1.consumed_energy:.0E} J")

    this_actor.info("Sleep for 10 seconds")
    this_actor.sleep_for(10)
    this_actor.info(f"Done sleeping. Current pstate: {host1.pstate}; "
                    f"Energy dissipated so far: {host1.consumed_energy:.2f} J")

    simulate_shutdown(host1)
    this_actor.info(f"Host1 is now OFF. Current pstate: {host1.pstate}; "
                    f"Energy dissipated so far: {host1.consumed_energy:.2f} J")

    this_actor.info("Sleep for 10 seconds")
    this_actor.sleep_for(10)
    this_actor.info(f"Done sleeping. Current pstate: {host1.pstate}; "
                    f"Energy dissipated so far: {host1.consumed_energy:.2f} J")

    simulate_bootup(host1)
    this_actor.info(f"Host1 is now ON again. Current pstate: {host1.pstate}; "
                    f"Energy dissipated so far: {host1.consumed_energy:.2f} J")


if __name__ == '__main__':
    sg_host_energy_plugin_init()
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file [other parameters]")
    e.load_platform(sys.argv[1])

    e.host_by_name("MyHost2").add_actor("Boot Monitor", monitor)

    e.run()

    this_actor.info("End of simulation.")
