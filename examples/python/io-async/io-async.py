# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how to use raw disk accesses asynchronously: waiting for them (with or without timeout), testing
them, cancelling them, or detaching them.

Usage: io-async.py platform_file [other parameters]
"""

import sys
from simgrid import Engine, Host, IoOpType, TimeoutException, this_actor


def test(size):
    disk = Host.current().disks[0]
    this_actor.info(f"Hello! read {size} bytes from {disk.name}")

    activity = disk.io_init(size, IoOpType.READ)
    activity.start()
    activity.wait()

    this_actor.info("Goodbye now!")


def test_detach(size):
    disk = Host.current().disks[0]
    this_actor.sleep_for(0.2)
    this_actor.info(f"Hello! read {size} bytes from {disk.name}")

    activity = disk.io_init(size, IoOpType.READ)
    # Attach a callback to print some log when the detached activity completes
    activity.on_this_completion_cb(lambda io: this_actor.info("Detached activity is done"))
    activity.detach()

    this_actor.info("Goodbye now!")


def test_waitfor(size):
    disk = Host.current().disks[0]
    this_actor.info(f"Hello! write {size} bytes from {disk.name}")

    activity = disk.write_async(size)
    try:
        activity.wait_for(0.5)
    except TimeoutException:
        this_actor.info("Asynchronous write: Timeout!")

    this_actor.info("Goodbye now!")


def test_cancel(size):
    disk = Host.current().disks[0]
    this_actor.sleep_for(0.5)
    this_actor.info(f"Hello! write {size} bytes from {disk.name}")

    activity = disk.write_async(size)
    this_actor.sleep_for(0.5)
    this_actor.info("I changed my mind, cancel!")
    activity.cancel()

    this_actor.info("Goodbye now!")


def test_monitor(size):
    disk = Host.current().disks[0]
    this_actor.sleep_for(1)
    activity = disk.write_async(size)

    while not activity.test():
        this_actor.info(f"Remaining amount of bytes to write: {activity.remaining:g}")
        this_actor.sleep_for(0.2)
    activity.wait()

    this_actor.info("Goodbye now!")


if __name__ == '__main__':
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file [other parameters]")
    e.load_platform(sys.argv[1])

    e.host_by_name("bob").add_actor("test", test, 20000000)
    e.host_by_name("bob").add_actor("test_detach", test_detach, 20000000)
    e.host_by_name("alice").add_actor("test_waitfor", test_waitfor, 50000000)
    e.host_by_name("alice").add_actor("test_cancel", test_cancel, 50000000)
    e.host_by_name("alice").add_actor("test_monitor", test_monitor, 50000000)

    e.run()

    this_actor.info(f"Simulation time {e.clock:g}")
