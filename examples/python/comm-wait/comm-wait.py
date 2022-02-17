# Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.

#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how to use simgrid::s4u::this_actor::wait() to wait for a given communication.

As for the other asynchronous examples, the sender initiate all the messages it wants to send and
pack the resulting simgrid::s4u::CommPtr objects in a vector. All messages thus occurs concurrently.

The sender then loops until there is no ongoing communication.
"""

import sys
from simgrid import Actor, Engine, Host, Mailbox, this_actor


def sender(messages_count, msg_size, receivers_count):
    # List in which we store all ongoing communications
    pending_comms = []

    # Vector of the used mailboxes
    mboxes = [Mailbox.by_name("receiver-{:d}".format(i)) for i in range(0, receivers_count)]

    # Start dispatching all messages to receivers, in a round robin fashion
    for i in range(0, messages_count):
        content = "Message {:d}".format(i)
        mbox = mboxes[i % receivers_count]

        this_actor.info("Send '{:s}' to '{:s}'".format(content, str(mbox)))

        # Create a communication representing the ongoing communication, and store it in pending_comms
        comm = mbox.put_async(content, msg_size)
        pending_comms.append(comm)

    # Start sending messages to let the workers know that they should stop
    for i in range(0, receivers_count):
        mbox = mboxes[i]
        this_actor.info("Send 'finalize' to '{:s}'".format(str(mbox)))
        comm = mbox.put_async("finalize", 0)
        pending_comms.append(comm)

    this_actor.info("Done dispatching all messages")

    # Now that all message exchanges were initiated, wait for their completion, in order of creation.
    for comm in pending_comms:
        comm.wait()
    this_actor.info("Goodbye now!")

def receiver(my_id):
    mbox = Mailbox.by_name("receiver-{:d}".format(my_id))
    this_actor.info("Wait for my first message")
    while True:
        received = mbox.get()
        this_actor.info("I got a '{:s}'.".format(received))
        if received == "finalize":
            break  # If it's a finalize message, we're done.


if __name__ == '__main__':
    e = Engine(sys.argv)

    e.load_platform(sys.argv[1])             # Load the platform description

    Actor.create("sender", Host.by_name("Tremblay"), sender, 3, 50000000, 1)
    Actor.create("receiver", Host.by_name("Ruby"), receiver, 0)

    e.run()
