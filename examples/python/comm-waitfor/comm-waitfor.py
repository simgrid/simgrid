# Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how to block on the completion of a set of communications.

As for the other asynchronous examples, the sender initiate all the messages it wants to send and
pack the resulting simgrid.Comm objects in a list. All messages thus occur concurrently.

The sender then loops until there is no ongoing communication. Using wait_any() ensures that the sender
will notice events as soon as they occur even if it does not follow the order of the container.

Here, finalize messages will terminate earlier because their size is 0, so they travel faster than the
other messages of this application.  As expected, the trace shows that the finalize of worker 1 is
processed before 'Message 5' that is sent to worker 0.
"""

import sys
from simgrid import Actor, Comm, Engine, Host, Mailbox, this_actor


FINALIZE_TAG = "finalize"


def sender(receiver_id: str, messages_count: int, payload_size: int) -> None:
    # List in which we store all ongoing communications
    pending_comms = []
    mbox = Mailbox.by_name(receiver_id)

    # Asynchronously send `messages_count` message(s) to the receiver
    for i in range(0, messages_count):
        payload = "Message {:d}".format(i)
        this_actor.info("Send '{:s}' to '{:s}'".format(payload, receiver_id))

        # Create a communication representing the ongoing communication, and store it in pending_comms
        comm = mbox.put_async(payload, payload_size)
        pending_comms.append(comm)

    # Send the final message to the receiver
    payload = FINALIZE_TAG
    final_payload_size = 0
    final_comm = mbox.put_async(payload, final_payload_size)
    pending_comms.append(final_comm)
    this_actor.info("Send '{:s}' to '{:s}".format(payload, receiver_id))
    this_actor.info("Done dispatching all messages")

    this_actor.info("Waiting for all outstanding communications to complete")
    while pending_comms:
        current_comm: Comm = pending_comms[-1]
        current_comm.wait_for(1.0)
        pending_comms.pop()
    this_actor.info("Goodbye now!")


def receiver(identifier: str) -> None:
    mbox: Mailbox = Mailbox.by_name(identifier)
    this_actor.info("Wait for my first message")
    while True:
        received = mbox.get()
        this_actor.info("I got a '{:s}'.".format(received))
        if received == FINALIZE_TAG:
            break
    this_actor.info("Goodbye now!")


def main():
    e = Engine(sys.argv)

    # Load the platform description
    e.load_platform(sys.argv[1])

    receiver_id = "receiver-0"
    Actor.create("sender", Host.by_name("Tremblay"), sender, receiver_id, 3, int(5e7))
    Actor.create("receiver", Host.by_name("Ruby"), receiver, receiver_id)

    e.run()


if __name__ == '__main__':
    main()
