# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how the energy consumed by the hosts is computed in case of parallel tasks (ptasks), that use
several hosts at once for computing and communicating.

Usage: energy-exec-ptask.py platform_file [other parameters]
"""

import sys
from simgrid import Engine, Host, TimeoutException, sg_host_energy_plugin_init, this_actor


class EnergyReporter:
    """Report the energy dissipated by the hosts since the previous report"""

    def __init__(self, hosts):
        self.hosts = hosts
        self.old_energy = [host.consumed_energy for host in hosts]

    def report(self, title, unit=" flop/s", total_format=".0f"):
        lines = [title]
        for i, host in enumerate(self.hosts):
            new_energy = host.consumed_energy
            lines.append(f"[{host.name}] Current peak speed={host.speed:.0E}{unit}; "
                         f"Energy dissipated during this step={new_energy - self.old_energy[i]:.2f} J; "
                         f"Total energy dissipated={new_energy:{total_format}} J")
            self.old_energy[i] = new_energy
        this_actor.info("\n".join(lines) + "\n")


def runner():
    host1 = Host.by_name("MyHost1")
    host2 = Host.by_name("MyHost2")
    hosts = [host1, host2]

    reporter = EnergyReporter(hosts)

    for host in hosts:
        this_actor.info(f"[{host.name}] Energetic profile: {host.get_property('wattage_per_state')}")
        this_actor.info(f"[{host.name}] Initial peak speed={host.speed:.0E} flop/s; "
                        f"Total energy dissipated ={host.consumed_energy:.0E} J")

    start = Engine.clock
    this_actor.info("Sleep for 10 seconds")
    this_actor.sleep_for(10)
    reporter.report(f"Done sleeping (duration: {Engine.clock - start:.2f} s).", unit="", total_format=".2f")

    # ========= Execute something =========
    start = Engine.clock
    flop_amount = 1E9
    this_actor.info(f"Run a task of {flop_amount:.0E} flops on two hosts")
    this_actor.parallel_execute(hosts, [flop_amount, flop_amount], [0, 0, 0, 0])
    reporter.report(f"Task done (duration: {Engine.clock - start:.2f} s).")

    # ========= Change power peak =========
    pstate = 2
    host1.pstate = pstate
    host2.pstate = pstate
    this_actor.info(f"========= Requesting pstate {pstate} for both hosts (speed should be of "
                    f"{host1.pstate_speed(pstate):.0E} flop/s and is of {host1.speed:.0E} flop/s)")

    # ========= Run another ptask =========
    start = Engine.clock
    this_actor.info(f"Run a task of {flop_amount:.0E} flops on {host1.name} and {flop_amount:.0E} flops on {host2.name}.")
    this_actor.parallel_execute(hosts, [flop_amount, flop_amount], [0, 0, 0, 0])
    reporter.report(f"Task done (duration: {Engine.clock - start:.2f} s).")

    # ========= A new ptask with computation and communication =========
    start = Engine.clock
    com_amount = 1E7
    this_actor.info("Run a task with computation and communication on two hosts.")
    this_actor.parallel_execute(hosts, [flop_amount, flop_amount], [0, com_amount, com_amount, 0])
    reporter.report(f"Task done (duration: {Engine.clock - start:.2f} s).")

    # ========= A new ptask with communication only =========
    start = Engine.clock
    this_actor.info("Run a task with only communication on two hosts.")
    this_actor.parallel_execute(hosts, [0, 0], [0, com_amount, com_amount, 0])
    reporter.report(f"Task done (duration: {Engine.clock - start:.2f} s).")

    # ========= A new ptask with computation and a timeout =========
    start = Engine.clock
    this_actor.info("Run a task with computation on two hosts and a timeout of 20s.")
    try:
        this_actor.exec_init(hosts, [flop_amount, flop_amount], [0, 0, 0, 0]).wait_for(20)
    except TimeoutException:
        this_actor.info("Finished WITH timeout")
    reporter.report(f"Task ended (duration: {Engine.clock - start:.2f} s).")

    this_actor.info("Now is time to quit!")


if __name__ == '__main__':
    sg_host_energy_plugin_init()
    e = Engine(sys.argv)
    Engine.set_config("host/model:ptask_L07")
    if len(sys.argv) < 2:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file [other parameters]")
    e.load_platform(sys.argv[1])

    e.host_by_name("MyHost1").add_actor("energy_ptask_test", runner)

    e.run()
    this_actor.info("End of simulation.")
