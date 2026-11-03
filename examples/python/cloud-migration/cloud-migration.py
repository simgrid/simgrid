# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how to migrate virtual machines between hosts, with the live migration plugin.

Usage: cloud-migration.py platform_file [other parameters]
"""

import sys
from simgrid import Engine, Host, sg_vm_live_migration_plugin_init, this_actor


def vm_migrate(vm, dst_pm):
    src_pm = vm.pm
    mig_sta = Engine.clock
    vm.migrate(dst_pm)
    mig_end = Engine.clock

    this_actor.info(f"{vm.name} migrated: {src_pm.name}->{dst_pm.name} in {mig_end - mig_sta:g} s")


def vm_migrate_async(vm, dst_pm):
    Host.current().add_actor("mig_wrk", vm_migrate, vm, dst_pm)


def master_main():
    pm0 = Host.by_name("Fafard")
    pm1 = Host.by_name("Tremblay")
    pm2 = Host.by_name("Bourassa")

    vm0 = pm0.create_vm("VM0", 1)
    vm0.ramsize = 1000 * 1000 * 1000  # 1Gbytes
    vm0.start()

    this_actor.info(f"Test: Migrate a VM with {vm0.ramsize // 1000 // 1000} Mbytes RAM")
    vm_migrate(vm0, pm1)

    vm0.destroy()

    vm0 = pm0.create_vm("VM0", 1)
    vm0.ramsize = 100 * 1000 * 1000  # 100Mbytes
    vm0.start()

    this_actor.info(f"Test: Migrate a VM with {vm0.ramsize // 1000 // 1000} Mbytes RAM")
    vm_migrate(vm0, pm1)

    vm0.destroy()

    vm0 = pm0.create_vm("VM0", 1)
    vm1 = pm0.create_vm("VM1", 1)

    vm0.ramsize = 1000 * 1000 * 1000  # 1Gbytes
    vm1.ramsize = 1000 * 1000 * 1000  # 1Gbytes
    vm0.start()
    vm1.start()

    this_actor.info("Test: Migrate two VMs at once from PM0 to PM1")
    vm_migrate_async(vm0, pm1)
    vm_migrate_async(vm1, pm1)
    this_actor.sleep_for(10000)

    vm0.destroy()
    vm1.destroy()

    vm0 = pm0.create_vm("VM0", 1)
    vm1 = pm0.create_vm("VM1", 1)

    vm0.ramsize = 1000 * 1000 * 1000  # 1Gbytes
    vm1.ramsize = 1000 * 1000 * 1000  # 1Gbytes
    vm0.start()
    vm1.start()

    this_actor.info("Test: Migrate two VMs at once to different PMs")
    vm_migrate_async(vm0, pm1)
    vm_migrate_async(vm1, pm2)
    this_actor.sleep_for(10000)

    vm0.destroy()
    vm1.destroy()


if __name__ == '__main__':
    e = Engine(sys.argv)
    sg_vm_live_migration_plugin_init()
    if len(sys.argv) < 2:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file [other parameters]")
    e.load_platform(sys.argv[1])

    Host.by_name("Fafard").add_actor("master_", master_main)

    e.run()

    this_actor.info(f"Bye (simulation time {e.clock:g})")
