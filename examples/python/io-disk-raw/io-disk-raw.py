# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how to simply read and write data on a simulated disk resource.

Usage: io-disk-raw.py platform_file [other parameters]
"""

import sys
from simgrid import Engine, Host, this_actor


def host():
    # Add an extra disk in a programmatic way
    Host.current().add_disk("Disk3", 9.6e7, 6.4e7).seal()  # read bandwidth, write bandwidth

    # Display information on the disks mounted by the current host
    this_actor.info(f"*** Storage info on {Host.current().name} ***")

    # Retrieve all disks from current host
    disk_list = Host.current().disks

    # For each disk mounted on host, display disk name and bandwidths
    for disk in disk_list:
        this_actor.info(f"Disk name: {disk.name} (read: {disk.read_bandwidth:.0f} B/s -- "
                        f"write: {disk.write_bandwidth:.0f} B/s")

    # Write 400,000 bytes on Disk1
    disk = disk_list[0]
    written = disk.write(400000)
    this_actor.info(f"Wrote {written} bytes on '{disk.name}'")

    # Now read 200,000 bytes
    read = disk.read(200000)
    this_actor.info(f"Read {read} bytes on '{disk.name}'")

    # Write 800,000 bytes on Disk3
    disk3 = disk_list[-1]
    written_on_disk3 = disk3.write(800000)
    this_actor.info(f"Wrote {written_on_disk3} bytes on '{disk3.name}'")

    # Attach some user data to disk1
    this_actor.info("*** Get/set data for storage element: Disk1 ***")

    this_actor.info(f"Get storage data: '{disk.data if disk.data is not None else 'No user data'}'")

    disk.data = "Some user data"
    this_actor.info(f"Set and get data: '{disk.data}'")


if __name__ == '__main__':
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file [other parameters]")
    e.load_platform(sys.argv[1])

    # Display Host properties
    for h in e.all_hosts:
        this_actor.info(f"*** {h.name} properties ****")
        for key, value in h.get_properties().items():
            this_actor.info(f"  {key} -> {value}")

    e.host_by_name("bob").add_actor("", host)

    e.run()
    this_actor.info(f"Simulated time: {e.clock:g}")
