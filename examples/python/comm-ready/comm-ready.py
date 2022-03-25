# Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

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


def get_peer_mailbox(peer_id: int) -> Mailbox:
    return Mailbox.by_name(f"peer-{peer_id}")


def peer(my_id: int, message_count: int, payload_size: int, peers_count: int):
    my_mailbox: Mailbox = get_peer_mailbox(my_id)
    my_mailbox.set_receiver(Actor.self())
    pending_comms: List[Comm] = []
    # Start dispatching all messages to peers others that myself
    for i in range(message_count):
        for peer_id in range(peers_count):
            if peer_id != my_id:
                peer_mailbox = get_peer_mailbox(peer_id)
                message = f"Message {i} from peer {my_id}"
                this_actor.info(f"Send '{message}' to '{peer_mailbox.name}'")
                pending_comms.append(peer_mailbox.put_async(message, payload_size))

    # Start sending messages to let peers know that they should stop
    for peer_id in range(peers_count):
        if peer_id != my_id:
            peer_mailbox = get_peer_mailbox(peer_id)
            payload = str(FINALIZE_MESSAGE)
            pending_comms.append(peer_mailbox.put_async(payload, payload_size))
            this_actor.info(f"Send '{payload}' to '{peer_mailbox.name}'")
    this_actor.info("Done dispatching all messages")

    # Retrieve all the messages other peers have been sending to me until I receive all the corresponding "Finalize"
    # messages
    pending_finalize_messages = peers_count - 1
    while pending_finalize_messages > 0:
        if my_mailbox.ready:
            start = Engine.clock
            received: str = my_mailbox.get()
            waiting_time = Engine.clock - start
            if waiting_time != 0.0:
                raise AssertionError(f"Expecting the waiting time to be 0.0 because the communication was supposedly "
                                     f"ready, but got {waiting_time} instead")
            this_actor.info(f"I got a '{received}'.")
            if received == FINALIZE_MESSAGE:
                pending_finalize_messages -= 1
        else:
            this_actor.info("Nothing ready to consume yet, I better sleep for a while")
            this_actor.sleep_for(0.01)

    this_actor.info("I'm done, just waiting for my peers to receive the messages before exiting")
    Comm.wait_all(pending_comms)
    this_actor.info("Goodbye now!")


def main():
    settings = create_parser().parse_known_args()[0]
    e = Engine(sys.argv)
    e.load_platform(settings.platform)
    Actor.create("peer", e.host_by_name("Tremblay"), peer, 0, 2, int(5e7), 3)
    Actor.create("peer", e.host_by_name("Ruby"), peer, 1, 6, int(2.5e5), 3)
    Actor.create("peer", e.host_by_name("Perl"), peer, 2, 0, int(5e7), 3)
    e.run()


if __name__ == "__main__":
    main()
