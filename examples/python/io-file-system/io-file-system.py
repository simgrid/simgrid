# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example illustrates the use of the file system plugin, with the classical operations on files (open, read,
write, seek, move, unlink, etc).

Usage: io-file-system.py platform_file [other parameters]
"""

import sys
from simgrid import Engine, File, Host, this_actor, sg_storage_file_system_init


class MyHost:
    @staticmethod
    def show_info(disks):
        this_actor.info(f"Storage info on {Host.current().name}:")

        for d in disks:
            # Retrieve disk's information
            this_actor.info(f"    {d.name} ({d.mount_point}) Used: {d.size_used}; Free: {d.size_free}; "
                            f"Total: {d.size}.")

    def __call__(self):
        disks = Host.current().disks

        self.show_info(disks)

        # Open a non-existing file to create it
        filename = "/scratch/tmp/data.txt"
        with File.open(filename) as file:
            written = file.write(200000)  # Write 200,000 bytes
            this_actor.info(f"Create a {written} bytes file named '{filename}' on /scratch")

            # check that sizes have changed
            self.show_info(disks)

            # Now retrieve the size of created file and read it completely
            file_size = file.size
            file.seek(0)
            read = file.read(file_size)
            this_actor.info(f"Read {read} bytes on {filename}")

            # Now write 100,000 bytes in tmp/data.txt
            written = file.write(100000)  # Write 100,000 bytes
            this_actor.info(f"Write {written} bytes on {filename}")

            # Now rename file from ./tmp/data.txt to ./tmp/simgrid.readme
            newpath = "/scratch/tmp/simgrid.readme"
            this_actor.info(f"Move '{file.path}' to '{newpath}'")
            file.move(newpath)

            # Test attaching some user data to the file
            file.data = "777"
            this_actor.info(f"User data attached to the file: {file.data}")
        # The file is automatically closed when leaving the 'with' block

        self.show_info(disks)

        # Reopen the file and then unlink it
        with File.open("/scratch/tmp/simgrid.readme") as file:
            this_actor.info(f"Unlink file: '{file.path}'")
            file.unlink()  # Unlinking the file on "disk" does not close the file

        self.show_info(disks)

        # Open another file on disk without a "content" property
        filename = "/lib/libc.so"
        with File.open(filename) as file:
            written = file.write(4096)  # Write 4 Kbytes
            this_actor.info(f"Create a {written} bytes file named '{filename}' on /")

        self.show_info(disks)


if __name__ == '__main__':
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file [other parameters]")
    sg_storage_file_system_init()
    e.load_platform(sys.argv[1])

    e.host_by_name("bob").add_actor("host", MyHost())

    e.run()
