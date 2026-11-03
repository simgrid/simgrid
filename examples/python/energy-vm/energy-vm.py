# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how the energy consumed by the hosts is computed when they host virtual machines.

Usage: energy-vm.py platform_file [other parameters]
"""

import sys
from simgrid import Engine, Host, sg_host_energy_plugin_init, this_actor


def executor():
    this_actor.execute(300E6)
    this_actor.info("This worker is done.")


def dvfs():
    host1 = Host.by_name("MyHost1")
    host2 = Host.by_name("MyHost2")
    host3 = Host.by_name("MyHost3")

    # Host 1
    this_actor.info("Creating and starting two VMs")
    vm_host1 = host1.create_vm("vm_host1", 1)
    vm_host1.start()
    vm_host2 = host2.create_vm("vm_host2", 1)
    vm_host2.start()

    this_actor.info("Create two activities on Host1: both inside a VM")
    vm_host1.add_actor("p11", executor)
    vm_host1.add_actor("p12", executor)

    this_actor.info("Create two activities on Host2: one inside a VM, the other directly on the host")
    vm_host2.add_actor("p21", executor)
    host2.add_actor("p22", executor)

    this_actor.info("Create two activities on Host3: both directly on the host")
    host3.add_actor("p31", executor)
    host3.add_actor("p32", executor)

    this_actor.info("Wait 5 seconds. The activities are still running (they run for 3 seconds, but 2 activities are "
                    "co-located, so they run for 6 seconds)")
    this_actor.sleep_for(5)
    this_actor.info("Wait another 5 seconds. The activities stop at some point in between")
    this_actor.sleep_for(5)

    vm_host1.destroy()
    vm_host2.destroy()


if __name__ == '__main__':
    sg_host_energy_plugin_init()
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file [other parameters]")
    e.load_platform(sys.argv[1])

    e.host_by_name("MyHost1").add_actor("dvfs", dvfs)

    e.run()

    this_actor.info(f"Total simulation time: {e.clock:.2f}; Host2 and Host3 must have the exact same energy "
                    "consumption; Host1 is multi-core and will differ.")
