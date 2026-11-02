# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how to specify the priority of a disk access: an activity with priority 2 is twice as fast as a
regular one, of priority 1.

Usage: io-priority.py platform_file [other parameters]
"""

import sys
from simgrid import Engine, Host, this_actor


def writer():
    # Retrieve all disks from current host
    disk_list = Host.current().disks
    # Write 4,000,000 bytes on Disk1
    disk_list[0].write(4000000)
    this_actor.info("First write done.")
    # Write 4,000,000 bytes on Disk1 again
    disk_list[0].write(4000000)
    this_actor.info("Second write done.")


def privileged_writer():
    # Retrieve all disks from current host
    disk_list = Host.current().disks

    # Write 4,000,000 bytes on Disk1 but specifies that this I/O operation gets a larger share of the resource.
    #
    # Since the priority is 2, it writes twice as fast as a regular one.
    #
    # So instead of a half/half sharing between the two, we get a 1/3 vs. 2/3 sharing.
    disk_list[0].write(4000000, priority=2)
    this_actor.info("First write done.")

    # Note that the timings printed when running this example are a bit misleading, because the uneven sharing only
    # last until the privileged actor ends. After this point, the unprivileged one gets 100% of the disk and finishes
    # quite quickly.

    # Resynchronize actors before second write
    this_actor.sleep_for(0.05)

    # Write 4,000,000 bytes on Disk1 again and this time:
    #  - Start the I/O operation asynchronously to get an Io activity
    #  - Let the actor sleep while half of the data is written
    #  - Double its priority
    #  - Wait for the end of the I/O operation
    #
    # Writing the second half of the data with a higher priority, and thus 2/3 of the bandwidth takes 0.075s.
    # In the meantime, the regular writer has only 1/3 of the bandwidth and thus only writes 1MB. After the completion
    # of the I/O initiated by the privileged writer, the regular writer has the full bandwidth available and only
    # needs 0.025s to write the last MB.
    io = disk_list[0].write_async(4000000)
    this_actor.sleep_for(0.1)
    this_actor.info(f"Increase priority for the priviledged writer ({io.remaining:.0f} bytes remaining to write)")
    io.update_priority(2)
    io.wait()
    this_actor.info("Second write done.")


if __name__ == '__main__':
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file [other parameters]")
    e.load_platform(sys.argv[1])

    e.host_by_name("bob").add_actor("writer", writer)
    e.host_by_name("bob").add_actor("privileged_writer", privileged_writer)

    e.run()
