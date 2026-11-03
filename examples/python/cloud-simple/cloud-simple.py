# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how to create virtual machines, run actors in them, and communicate with them.

Usage: cloud-simple.py platform_file [other parameters]
"""

import sys
from simgrid import Engine, Host, Mailbox, sg_vm_live_migration_plugin_init, this_actor


def computation_fun():
    clock_sta = Engine.clock
    this_actor.execute(1000000)
    clock_end = Engine.clock

    this_actor.info(f"{this_actor.get_host().name}:{this_actor.get_name()} executed {clock_end - clock_sta:g}")


def launch_computation_worker(host):
    host.add_actor("compute", computation_fun)


def communication_tx_fun(mbox_name):
    mbox = Mailbox.by_name(mbox_name)
    payload = (this_actor.get_host(), this_actor.get_name(), Engine.clock)
    mbox.put(payload, 1000000)


def communication_rx_fun(mbox_name):
    actor_name = this_actor.get_name()
    host_name = this_actor.get_host().name
    mbox = Mailbox.by_name(mbox_name)

    tx_host, tx_actor_name, clock_sta = mbox.get()
    clock_end = Engine.clock

    this_actor.info(f"{tx_host.name}:{tx_actor_name} to {host_name}:{actor_name} => {clock_end - clock_sta:g} sec")


def launch_communication_worker(tx_host, rx_host):
    mbox_name = f"MBOX:{tx_host.name}-{rx_host.name}"

    tx_host.add_actor("comm_tx", communication_tx_fun, mbox_name)
    rx_host.add_actor("comm_rx", communication_rx_fun, mbox_name)


def master_main():
    pm0 = Host.by_name("Fafard")
    pm1 = Host.by_name("Tremblay")
    pm2 = Host.by_name("Bourassa")

    this_actor.info("## Test 1 (started): check computation on normal PMs")

    this_actor.info("### Put an activity on a PM")
    launch_computation_worker(pm0)
    this_actor.sleep_for(2)

    this_actor.info("### Put two activities on a PM")
    launch_computation_worker(pm0)
    launch_computation_worker(pm0)
    this_actor.sleep_for(2)

    this_actor.info("### Put an activity on each PM")
    launch_computation_worker(pm0)
    launch_computation_worker(pm1)
    this_actor.sleep_for(2)

    this_actor.info("## Test 1 (ended)")

    this_actor.info(
        "## Test 2 (started): check impact of running an activity inside a VM (there is no degradation for the moment)")

    this_actor.info("### Put a VM on a PM, and put an activity to the VM")
    vm0 = pm0.create_vm("VM0", 1)
    vm0.start()
    launch_computation_worker(vm0)
    this_actor.sleep_for(2)
    vm0.destroy()

    this_actor.info("## Test 2 (ended)")

    this_actor.info("## Test 3 (started): check impact of running an activity collocated with a VM (there is no VM "
                    "noise for the moment)")

    this_actor.info("### Put a VM on a PM, and put an activity to the PM")
    vm0 = pm0.create_vm("VM0", 1)
    vm0.start()
    launch_computation_worker(pm0)
    this_actor.sleep_for(2)
    vm0.destroy()
    this_actor.info("## Test 3 (ended)")

    this_actor.info(
        "## Test 4 (started): compare the cost of running two activities inside two different VMs collocated or not "
        "(for the moment, there is no degradation for the VMs. Hence, the time should be equals to the time of test 1")

    this_actor.info("### Put two VMs on a PM, and put an activity to each VM")
    vm0 = pm0.create_vm("VM0", 1)
    vm0.start()
    vm1 = pm0.create_vm("VM1", 1)
    vm1.start()
    launch_computation_worker(vm0)
    launch_computation_worker(vm1)
    this_actor.sleep_for(2)
    vm0.destroy()
    vm1.destroy()

    this_actor.info("### Put a VM on each PM, and put an activity to each VM")
    vm0 = pm0.create_vm("VM0", 1)
    vm1 = pm1.create_vm("VM1", 1)
    vm0.start()
    vm1.start()
    launch_computation_worker(vm0)
    launch_computation_worker(vm1)
    this_actor.sleep_for(2)
    vm0.destroy()
    vm1.destroy()
    this_actor.info("## Test 4 (ended)")

    this_actor.info("## Test 5  (started): Analyse network impact")
    this_actor.info("### Make a connection between PM0 and PM1")
    launch_communication_worker(pm0, pm1)
    this_actor.sleep_for(5)

    this_actor.info("### Make two connection between PM0 and PM1")
    launch_communication_worker(pm0, pm1)
    launch_communication_worker(pm0, pm1)
    this_actor.sleep_for(5)

    this_actor.info("### Make a connection between PM0 and VM0@PM0")
    vm0 = pm0.create_vm("VM0", 1)
    vm0.start()
    launch_communication_worker(pm0, vm0)
    this_actor.sleep_for(5)
    vm0.destroy()

    this_actor.info("### Make a connection between PM0 and VM0@PM1")
    vm0 = pm1.create_vm("VM0", 1)
    launch_communication_worker(pm0, vm0)
    this_actor.sleep_for(5)
    vm0.destroy()

    this_actor.info("### Make two connections between PM0 and VM0@PM1")
    vm0 = pm1.create_vm("VM0", 1)
    vm0.start()
    launch_communication_worker(pm0, vm0)
    launch_communication_worker(pm0, vm0)
    this_actor.sleep_for(5)
    vm0.destroy()

    this_actor.info("### Make a connection between PM0 and VM0@PM1, and also make a connection between PM0 and PM1")
    vm0 = pm1.create_vm("VM0", 1)
    vm0.start()
    launch_communication_worker(pm0, vm0)
    launch_communication_worker(pm0, pm1)
    this_actor.sleep_for(5)
    vm0.destroy()

    this_actor.info("### Make a connection between VM0@PM0 and PM1@PM1, and also make a connection between VM0@PM0 and "
                    "VM1@PM1")
    vm0 = pm0.create_vm("VM0", 1)
    vm1 = pm1.create_vm("VM1", 1)
    vm0.start()
    vm1.start()
    launch_communication_worker(vm0, vm1)
    launch_communication_worker(vm0, vm1)
    this_actor.sleep_for(5)
    vm0.destroy()
    vm1.destroy()

    this_actor.info("## Test 5 (ended)")
    this_actor.info("## Test 6 (started): Check migration impact (not yet implemented neither on the CPU resource nor "
                    "on the network one")
    this_actor.info("### Relocate VM0 between PM0 and PM1")
    vm0 = pm0.create_vm("VM0", 1)
    vm0.ramsize = 1024 * 1024 * 1024  # 1GiB

    vm0.start()
    launch_communication_worker(vm0, pm2)
    this_actor.sleep_for(0.01)
    vm0.migrate(pm1)
    this_actor.sleep_for(0.01)
    vm0.migrate(pm0)
    this_actor.sleep_for(5)
    vm0.destroy()
    this_actor.info("## Test 6 (ended)")


if __name__ == '__main__':
    e = Engine(sys.argv)
    sg_vm_live_migration_plugin_init()
    if len(sys.argv) < 2:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file [other parameters]")
    e.load_platform(sys.argv[1])

    e.host_by_name("Fafard").add_actor("master_", master_main)

    e.run()

    this_actor.info(f"Simulation time {e.clock:g}")
