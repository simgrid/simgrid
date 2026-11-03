# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how to bound the CPU usage of executions and of virtual machines.

Usage: cloud-capping.py platform_file [other parameters]
"""

import sys
from simgrid import Engine, Host, sg_vm_live_migration_plugin_init, this_actor


def worker(computation_amount, use_bound, bound):
    clock_start = Engine.clock

    execution = this_actor.exec_init(computation_amount)

    if use_bound:
        if bound < 1e-12:  # close enough to 0 without any floating precision surprise
            this_actor.info("bound == 0 means no capping (i.e., unlimited).")
        execution.set_bound(bound)
    execution.start()
    execution.wait()
    clock_end = Engine.clock
    duration = clock_end - clock_start
    flops_per_sec = computation_amount / duration

    if use_bound:
        this_actor.info(f"bound to {bound:f} => duration {duration:f} ({flops_per_sec:f} flops/s)")
    else:
        this_actor.info(f"not bound => duration {duration:f} ({flops_per_sec:f} flops/s)")


def worker_busy_loop(name, speed):
    exec_remain_prev = 1e11
    execution = this_actor.exec_async(exec_remain_prev)
    for i in range(10):
        if speed > 0:
            new_bound = (speed / 10) * i
            this_actor.info(f"set bound of VM1 to {new_bound:f}")
            this_actor.get_host().set_bound(new_bound)  # We are running on a VM
        this_actor.sleep_for(100)
        exec_remain_now = execution.remaining
        flops_per_sec = exec_remain_prev - exec_remain_now
        this_actor.info(f"{name}@{this_actor.get_host().name}: {flops_per_sec / 100:.0f} flops/s")
        exec_remain_prev = exec_remain_now
        this_actor.sleep_for(1)
    execution.wait()


def test_dynamic_change():
    pm0 = Host.by_name("Fafard")

    vm0 = pm0.create_vm("VM0", 1)
    vm1 = pm0.create_vm("VM1", 1)
    vm0.start()
    vm1.start()

    vm0.add_actor("worker0", worker_busy_loop, "Task0", -1)
    vm1.add_actor("worker1", worker_busy_loop, "Task1", pm0.speed)

    this_actor.sleep_for(3000)  # let the activities end
    vm0.destroy()
    vm1.destroy()


def test_one_activity(host):
    cpu_speed = host.speed
    computation_amount = cpu_speed * 10

    this_actor.info("### Test: with/without activity set_bound")

    this_actor.info(f"### Test: no bound for Task1@{host.name}")
    host.add_actor("worker0", worker, computation_amount, False, 0)

    this_actor.sleep_for(1000)

    this_actor.info(f"### Test: 50% for Task1@{host.name}")
    host.add_actor("worker0", worker, computation_amount, True, cpu_speed / 2)

    this_actor.sleep_for(1000)

    this_actor.info(f"### Test: 33% for Task1@{host.name}")
    host.add_actor("worker0", worker, computation_amount, True, cpu_speed / 3)

    this_actor.sleep_for(1000)

    this_actor.info(f"### Test: zero for Task1@{host.name} (i.e., unlimited)")
    host.add_actor("worker0", worker, computation_amount, True, 0)

    this_actor.sleep_for(1000)

    this_actor.info(f"### Test: 200% for Task1@{host.name} (i.e., meaningless)")
    host.add_actor("worker0", worker, computation_amount, True, cpu_speed * 2)

    this_actor.sleep_for(1000)


def test_two_activities(host_a, host_b):
    cpu_speed = host_a.speed
    assert cpu_speed == host_b.speed
    computation_amount = cpu_speed * 10
    host_a_name = host_a.name
    host_b_name = host_b.name

    # Each test is described by its title, and the bound of both activities (None meaning no bound)
    tests = [
        ("no bound for Task1@{a}, no bound for Task2@{b}", None, None),
        ("0 for Task1@{a}, 0 for Task2@{b} (i.e., unlimited)", 0, 0),
        ("50% for Task1@{a}, 50% for Task2@{b}", cpu_speed / 2, cpu_speed / 2),
        ("25% for Task1@{a}, 25% for Task2@{b}", cpu_speed / 4, cpu_speed / 4),
        ("75% for Task1@{a}, 100% for Task2@{b}", cpu_speed * 0.75, cpu_speed),
        ("no bound for Task1@{a}, 25% for Task2@{b}", None, cpu_speed / 4),
        ("75% for Task1@{a}, 25% for Task2@{b}", cpu_speed * 0.75, cpu_speed / 4),
    ]
    for title, bound_a, bound_b in tests:
        this_actor.info("### Test: " + title.format(a=host_a_name, b=host_b_name))
        host_a.add_actor("worker0", worker, computation_amount, bound_a is not None, bound_a or 0)
        host_b.add_actor("worker1", worker, computation_amount, bound_b is not None, bound_b or 0)

        this_actor.sleep_for(1000)


def master_main():
    pm0 = Host.by_name("Fafard")

    this_actor.info("# 1. Put a single activity on a PM.")
    test_one_activity(pm0)
    this_actor.info(".")

    this_actor.info("# 2. Put two activities on a PM.")
    test_two_activities(pm0, pm0)
    this_actor.info(".")

    vm0 = pm0.create_vm("VM0", 1)
    vm0.start()

    this_actor.info("# 3. Put a single activity on a VM.")
    test_one_activity(vm0)
    this_actor.info(".")

    this_actor.info("# 4. Put two activities on a VM.")
    test_two_activities(vm0, vm0)
    this_actor.info(".")

    vm0.destroy()

    vm0 = pm0.create_vm("VM0", 1)
    vm0.start()

    this_actor.info("# 6. Put an activity on a PM and an activity on a VM.")
    test_two_activities(pm0, vm0)
    this_actor.info(".")

    vm0.destroy()

    vm0 = pm0.create_vm("VM0", 1)
    vm0.set_bound(pm0.speed / 10)
    vm0.start()

    this_actor.info("# 7. Put a single activity on the VM capped by 10%.")
    test_one_activity(vm0)
    this_actor.info(".")

    this_actor.info("# 8. Put two activities on the VM capped by 10%.")
    test_two_activities(vm0, vm0)
    this_actor.info(".")

    this_actor.info("# 9. Put an activity on a PM and an activity on the VM capped by 10%.")
    test_two_activities(pm0, vm0)
    this_actor.info(".")

    vm0.destroy()

    vm0 = pm0.create_vm("VM0", 1)
    vm0.ramsize = 1000 * 1000 * 1000  # 1GB
    vm0.start()

    cpu_speed = pm0.speed

    this_actor.info("# 10. Test migration")
    computation_amount = cpu_speed * 10

    this_actor.info("# 10. (a) Put an activity on a VM without any bound.")
    vm0.add_actor("worker0", worker, computation_amount, False, 0)
    this_actor.sleep_for(1000)
    this_actor.info(".")

    this_actor.info("# 10. (b) set 10% bound to the VM, and then put an activity on the VM.")
    vm0.set_bound(cpu_speed / 10)
    vm0.add_actor("worker0", worker, computation_amount, False, 0)
    this_actor.sleep_for(1000)
    this_actor.info(".")

    this_actor.info("# 10. (c) migrate")
    pm1 = Host.by_name("Fafard")
    vm0.migrate(pm1)
    this_actor.info(".")

    this_actor.info("# 10. (d) Put an activity again on the VM.")
    vm0.add_actor("worker0", worker, computation_amount, False, 0)
    this_actor.sleep_for(1000)
    this_actor.info(".")

    vm0.destroy()

    this_actor.info("# 11. Change a bound dynamically.")
    test_dynamic_change()


if __name__ == '__main__':
    e = Engine(sys.argv)
    sg_vm_live_migration_plugin_init()
    if len(sys.argv) < 2:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file [other parameters]")
    e.load_platform(sys.argv[1])

    e.host_by_name("Fafard").add_actor("master_", master_main)

    e.run()
    this_actor.info(f"Bye (simulation time {e.clock:g})")
