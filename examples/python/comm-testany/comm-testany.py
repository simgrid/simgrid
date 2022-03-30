# Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

from argparse import ArgumentParser
from typing import List
import sys

from simgrid import Engine, Actor, Comm, Mailbox, this_actor


def create_parser() -> ArgumentParser:
    parser = ArgumentParser()
    parser.add_argument(
        '--platform',
        type=str,
        required=True,
        help='path to the platform description'
    )
    return parser


def rank0():
    rank0_mailbox: Mailbox = Mailbox.by_name("rank0")
    this_actor.info("Post my asynchronous receives")
    comm1, a1 = rank0_mailbox.get_async()
    comm2, a2 = rank0_mailbox.get_async()
    comm3, a3 = rank0_mailbox.get_async()
    pending_comms: List[Comm] = [comm1, comm2, comm3]

    this_actor.info("Send some data to rank-1")
    rank1_mailbox: Mailbox = Mailbox.by_name("rank1")
    for i in range(3):
        rank1_mailbox.put(i, 1)

    this_actor.info("Test for completed comms")
    while pending_comms:
        flag = Comm.test_any(pending_comms)
        if flag != -1:
            pending_comms.pop(flag)
            this_actor.info("Remove a pending comm.")
        else:
            # Nothing matches, wait for a little bit
            this_actor.sleep_for(0.1)
    this_actor.info("Last comm is complete")


def rank1():
    rank0_mailbox: Mailbox = Mailbox.by_name("rank0")
    rank1_mailbox: Mailbox = Mailbox.by_name("rank1")
    for i in range(3):
        data: int = rank1_mailbox.get()
        this_actor.info(f"Received {data}")
        msg_content = f"Message {i}"
        this_actor.info(f"Send '{msg_content}'")
        rank0_mailbox.put(msg_content, int(1e6))


def main():
    settings = create_parser().parse_known_args()[0]
    e = Engine(sys.argv)
    e.load_platform(settings.platform)

    Actor.create("rank0", e.host_by_name("Tremblay"), rank0)
    Actor.create("rank1", e.host_by_name("Fafard"), rank1)

    e.run()


if __name__ == "__main__":
    main()
