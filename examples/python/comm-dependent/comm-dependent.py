# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how to declare dependencies between communications and executions: the sender computes before
sending the result, and the receiver computes once it got the data.

Usage: comm-dependent.py platform_file [other parameters]
"""

import sys
from simgrid import Engine, this_actor


def sender(mailbox):
    computation_amount = this_actor.get_host().speed
    execution = this_actor.exec_init(2 * computation_amount)
    comm = mailbox.put_init(computation_amount, 7_000_000)

    execution.name = "exec on sender"
    execution.add_successor(comm)
    execution.start()
    comm.name = "comm to receiver"
    comm.start()
    execution.wait()
    comm.wait()


def receiver(mailbox):
    computation_amount = this_actor.get_host().speed
    execution = this_actor.exec_init(2 * computation_amount)
    comm = mailbox.get_init()

    comm.name = "comm from sender"
    comm.add_successor(execution)
    comm.start()
    execution.name = "exec on receiver"
    execution.start()

    comm.wait()
    execution.wait()
    this_actor.info(f"Received: {comm.get_payload():.0f} flops were computed on sender")


if __name__ == '__main__':
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file [other parameters]")
    e.load_platform(sys.argv[1])

    mbox = e.mailbox_by_name_or_create("Mailbox")

    e.host_by_name("Tremblay").add_actor("sender", sender, mbox)
    e.host_by_name("Jupiter").add_actor("receiver", receiver, mbox)

    e.run()

    this_actor.info(f"Simulation time: {e.clock:.3f}")
