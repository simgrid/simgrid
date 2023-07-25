# Copyright (c) 2010-2023. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

from typing import List, Tuple
import sys

from simgrid import Engine, Actor, ActivitySet, Comm, Host, LinkInRoute, Mailbox, NetZone, this_actor


RECEIVER_MAILBOX_NAME = "receiver"


class Sender(object):
    def __init__(self, message_size: int, messages_count: int):
        self.message_size = message_size
        self.messages_count = messages_count

    def __call__(self) -> None:
        # List in which we store all ongoing communications
        pending_comms = ActivitySet()

        # Make a vector of the mailboxes to use
        receiver_mailbox: Mailbox = Mailbox.by_name(RECEIVER_MAILBOX_NAME)
        for i in range(self.messages_count):
            message_content = f"Message {i}"
            this_actor.info(f"Send '{message_content}' to '{receiver_mailbox.name}'")
            # Create a communication representing the ongoing communication, and store it in pending_comms
            pending_comms.push(receiver_mailbox.put_async(message_content, self.message_size))

        this_actor.info("Done dispatching all messages")

        # Now that all message exchanges were initiated, wait for their completion in one single call
        pending_comms.wait_all()

        this_actor.info("Goodbye now!")


class Receiver(object):
    def __init__(self, messages_count: int):
        self.mailbox: Mailbox = Mailbox.by_name(RECEIVER_MAILBOX_NAME)
        self.messages_count = messages_count

    def __call__(self):
        # List in which we store all incoming msgs
        pending_comms = ActivitySet()
        this_actor.info(f"Wait for {self.messages_count} messages asynchronously")
        for _ in range(self.messages_count):
            pending_comms.push(self.mailbox.get_async())
        while not pending_comms.empty():
            comm = pending_comms.wait_any()
            this_actor.info(f"I got '{comm.get_payload()}'.")


def main():
    e = Engine(sys.argv)
    # Creates the platform
    #  ________                 __________
    # | Sender |===============| Receiver |
    # |________|    Link1      |__________|
    #
    zone: NetZone = NetZone.create_full_zone("Zone1")
    sender_host: Host = zone.create_host("sender", 1).seal()
    receiver_host: Host = zone.create_host("receiver", 1).seal()

    # create split-duplex link1 (UP/DOWN), limiting the number of concurrent flows in it for 2
    link = zone.create_split_duplex_link("link1", 10e9).set_latency(10e-6).set_concurrency_limit(2).seal()

    # create routes between nodes
    zone.add_route(sender_host, receiver_host, [link])
    zone.seal()

    # create actors Sender/Receiver
    messages_count = 10
    Actor.create("receiver", receiver_host, Receiver(messages_count=messages_count))
    Actor.create("sender", sender_host, Sender(messages_count=messages_count, message_size=int(1e6)))

    e.run()


if __name__ == "__main__":
    main()
