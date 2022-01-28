# Copyright (c) 2006-2022. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

# This example shows how to simulate a non-linear resource sharing for disk
# operations.
#
# It is inspired on the paper
# "Adding Storage Simulation Capacities to the SimGridToolkit: Concepts, Models, and API"
# Available at : https://hal.inria.fr/hal-01197128/document
#
# It shows how to simulate concurrent operations degrading overall performance of IO
# operations (specifically the effects presented in Fig. 8 of the paper).


from simgrid import Actor, Engine, NetZone, Host, Disk, this_actor
import sys
import functools


def estimate_bw(disk: Disk, n_flows: int, read: bool):
    """ Calculates the bandwidth for disk doing async operations """
    size = 100000
    cur_time = Engine.clock
    activities = [disk.read_async(size) if read else disk.write_async(
        size) for _ in range(n_flows)]

    for act in activities:
        act.wait()

    elapsed_time = Engine.clock - cur_time
    estimated_bw = float(size * n_flows) / elapsed_time
    this_actor.info("Disk: %s, concurrent %s: %d, estimated bandwidth: %f" % (
        disk.name, "read" if read else "write", n_flows, estimated_bw))


def host():
    # Estimating bw for each disk and considering concurrent flows
    for n in range(1, 15, 2):
        for disk in Host.current().get_disks():
            estimate_bw(disk, n, True)
            estimate_bw(disk, n, False)


def ssd_dynamic_sharing(disk: Disk, op: str, capacity: float, n: int) -> float:
    """
    Non-linear resource callback for SSD disks

    In this case, we have measurements for some resource sharing and directly use them to return the
    correct value
    :param disk: Disk on which the operation is happening (defined by the user through the std::bind)
    :param op: read or write operation (defined by the user through the std::bind)
    :param capacity: Resource current capacity in SimGrid
    :param n: Number of activities sharing this resource
    """
    # measurements for SSD disks
    speed = {
        "write": {1: 131.},
        "read": {1: 152., 2: 161., 3: 184., 4: 197., 5: 207., 6: 215., 7: 220., 8: 224., 9: 227., 10: 231., 11: 233., 12: 235., 13: 237., 14: 238., 15: 239.}
    }

    # no special bandwidth for this disk sharing N flows, just returns maximal capacity
    if (n in speed[op]):
        capacity = speed[op][n]

    return capacity


def sata_dynamic_sharing(disk: Disk, capacity: float, n: int) -> float:
    """
    Non-linear resource callback for SATA disks

    In this case, the degradation for read operations is linear and we have a formula that represents it.

    :param disk: Disk on which the operation is happening (defined by the user through the std::bind)
    :param capacity: Resource current capacity in SimGrid
    :param n: Number of activities sharing this resource
    :return: New disk capacity
    """
    return 68.3 - 1.7 * n


def create_ssd_disk(host: Host, disk_name: str):
    """ Creates an SSD disk, setting the appropriate callback for non-linear resource sharing """
    disk = host.create_disk(disk_name, "240MBps", "170MBps")
    disk.set_sharing_policy(Disk.Operation.READ, Disk.SharingPolicy.NONLINEAR,
                            functools.partial(ssd_dynamic_sharing, disk, "read"))
    disk.set_sharing_policy(Disk.Operation.WRITE, Disk.SharingPolicy.NONLINEAR,
                            functools.partial(ssd_dynamic_sharing, disk, "write"))
    disk.set_sharing_policy(Disk.Operation.READWRITE,
                            Disk.SharingPolicy.LINEAR)


def create_sata_disk(host: Host, disk_name: str):
    """ Same for a SATA disk, only read operation follows a non-linear resource sharing """
    disk = host.create_disk(disk_name, "68MBps", "50MBps")
    disk.set_sharing_policy(Disk.Operation.READ, Disk.SharingPolicy.NONLINEAR,
                            functools.partial(sata_dynamic_sharing, disk))
    # this is the default behavior, expliciting only to make it clearer
    disk.set_sharing_policy(Disk.Operation.WRITE, Disk.SharingPolicy.LINEAR)
    disk.set_sharing_policy(Disk.Operation.READWRITE,
                            Disk.SharingPolicy.LINEAR)


if __name__ == '__main__':
    e = Engine(sys.argv)
    # simple platform containing 1 host and 2 disk
    zone = NetZone.create_full_zone("bob_zone")
    bob = zone.create_host("bob", 1e6)
    create_ssd_disk(bob, "Edel (SSD)")
    create_sata_disk(bob, "Griffon (SATA II)")
    zone.seal()

    Actor.create("runner", bob, host)

    e.run()
    this_actor.info("Simulated time: %g" % e.clock)

    # explicitly deleting Engine object to avoid segfault during cleanup phase.
    # During Engine destruction, the cleanup of std::function linked to non_linear callback is called.
    # If we let the cleanup by itself, it fails trying on its destruction because the python main program
    # has already freed its variables
    del(e)
