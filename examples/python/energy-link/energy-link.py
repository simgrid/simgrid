# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how to retrieve the energy consumed by the links.

Usage: energy-link.py platform_file [flow_count [data_size|random]] [other parameters]
"""

import random
import sys
from simgrid import ActivitySet, Engine, Mailbox, sg_link_energy_plugin_init, this_actor

# Parameters of the random generation of the flow size
MIN_SIZE = int(1e6)
MAX_SIZE = int(1e9)


def sender(flow_amount, comm_size):
    this_actor.info(f"Send {comm_size} bytes, in {flow_amount} flows")

    mailbox = Mailbox.by_name("message")

    # Sleep a while before starting the example
    this_actor.sleep_for(10)

    if flow_amount == 1:
        # Send the data to the receiver
        mailbox.put(str(comm_size), comm_size)
    else:
        # Start all comms in parallel, and wait for all completions in one shot
        comms = ActivitySet()
        for i in range(flow_amount):
            comms.push(mailbox.put_async(str(i), comm_size))
        comms.wait_all()
    this_actor.info("sender done.")


def receiver(flow_amount):
    this_actor.info(f"Receiving {flow_amount} flows ...")

    mailbox = Mailbox.by_name("message")

    if flow_amount == 1:
        mailbox.get()
    else:
        # Start all comms in parallel, and wait for their completion in one shot
        comms = ActivitySet()
        for _ in range(flow_amount):
            comms.push(mailbox.get_async())
        comms.wait_all()
    this_actor.info("receiver done.")


if __name__ == '__main__':
    e = Engine(sys.argv)

    this_actor.info("Activating the SimGrid link energy plugin")
    sg_link_energy_plugin_init()

    if len(sys.argv) < 2:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file [flow_count [data_size|random]]")
    e.load_platform(sys.argv[1])

    # Take the amount of flows and the data size from the command line, if any.
    # The SimGrid options (--cfg, --log) remain in sys.argv, so we ignore them here.
    args = [arg for arg in sys.argv[2:] if not arg.startswith("--")]
    flow_count = int(args[0]) if len(args) > 0 else 1
    if len(args) > 1:
        data_size = random.randint(MIN_SIZE, MAX_SIZE) if args[1] == "random" else int(args[1])
    else:
        data_size = 25000

    e.host_by_name("MyHost1").add_actor("sender", sender, flow_count, data_size)
    e.host_by_name("MyHost2").add_actor("receiver", receiver, flow_count)

    # And now, launch the simulation
    e.run()
