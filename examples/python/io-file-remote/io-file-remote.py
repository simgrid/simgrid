# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how to do I/O operations on files remotely, i.e. when the accessed disk is not mounted on the
caller's host. The actors are started from a deployment file.

Usage: io-file-remote.py platform_file deployment_file [other parameters]
"""

import sys
from simgrid import Engine, File, Host, this_actor, sg_storage_file_system_init

INMEGA = 1024 * 1024


def host(path, dest_host=None, dest_path=None, move=None):
    file = File.open(path)
    filename = file.path
    this_actor.info(f"Opened file '{filename}'")
    file.dump()
    this_actor.info(f"Try to write {file.size // 1024} MiB to '{filename}'")
    written = file.write(file.size * 1024)
    this_actor.info(f"Have written {written // INMEGA} MiB to '{filename}'.")

    if move is not None:
        if int(move) != 0:
            this_actor.info(f"Move '{filename}' (of size {file.size}) from '{Host.current().name}' to '{dest_host}'")
            file.remote_move(Host.by_name(dest_host), dest_path)
        else:
            this_actor.info(f"Copy '{filename}' (of size {file.size}) from '{Host.current().name}' to '{dest_host}'")
            file.remote_copy(Host.by_name(dest_host), dest_path)
    file.close()


if __name__ == '__main__':
    e = Engine(sys.argv)
    if len(sys.argv) < 3:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file deployment_file [other parameters]")
    sg_storage_file_system_init()
    e.load_platform(sys.argv[1])
    e.register_actor("host", host)
    e.load_deployment(sys.argv[2])
    all_hosts = e.all_hosts

    for h in all_hosts:
        for d in h.disks:
            this_actor.info(f"Init: {h.name}: {d.size_used // INMEGA}/{d.size_free // INMEGA} MiB used/free on "
                            f"'{d.name}@{d.host.name}'")

    e.run()

    for h in all_hosts:
        for d in h.disks:
            this_actor.info(f"End: {d.size_used // INMEGA}/{d.size_free // INMEGA} MiB used/free on "
                            f"'{d.name}@{h.name}'")

    this_actor.info(f"Simulation time {e.clock:g}")
