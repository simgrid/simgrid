# Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

""" This example shows how to suspend and resume an asynchronous communication.
"""

from argparse import ArgumentParser
from typing import List
import sys

from simgrid import Actor, Comm, Engine, Mailbox, this_actor


FINALIZE_MESSAGE = "finalize"


def create_parser() -> ArgumentParser:
    parser = ArgumentParser()
    parser.add_argument(
        '--platform',
        type=str,
        required=True,
        help='path to the platform description'
    )
    return parser


def sender(receiver_mailbox: Mailbox, messages_count: int, payload_size: int):
    pending_comms: List[Comm] = []
    # Start dispatching all messages to the receiver
    for i in range(messages_count):
        payload = f"Message {i}"
        this_actor.info(f"Send '{payload}' to '{receiver_mailbox.name}'")
        # Create a communication representing the ongoing communication
        comm = receiver_mailbox.put_async(payload, payload_size)
        # Add this comm to the vector of all known comms
        pending_comms.append(comm)

    # Start the finalize signal to the receiver
    final_comm = receiver_mailbox.put_async(FINALIZE_MESSAGE, 0)
    pending_comms.append(final_comm)
    this_actor.info(f"Send '{FINALIZE_MESSAGE}' to '{receiver_mailbox.name}'")
    this_actor.info("Done dispatching all messages")

    # Now that all message exchanges were initiated, wait for their completion, in order of creation
    while pending_comms:
        comm = pending_comms[-1]
        comm.wait_until(Engine.clock + 1)
        pending_comms.pop()  # remove it from the list
    this_actor.info("Goodbye now!")


def receiver(mailbox: Mailbox):
    this_actor.info("Wait for my first message")
    finalized = False
    while not finalized:
        received: str = mailbox.get()
        this_actor.info(f"I got a '{received}'.")
        # If it's a finalize message, we're done.
        if received == FINALIZE_MESSAGE:
            finalized = True


def main():
    settings = create_parser().parse_known_args()[0]
    e = Engine(sys.argv)
    e.load_platform(settings.platform)
    receiver_mailbox: Mailbox = Mailbox.by_name("receiver")
    Actor.create("sender", e.host_by_name("Tremblay"), sender, receiver_mailbox, 3, int(5e7))
    Actor.create("receiver", e.host_by_name("Ruby"), receiver, receiver_mailbox)
    e.run()


if __name__ == "__main__":
    main()
