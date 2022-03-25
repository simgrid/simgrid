# Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

from argparse import ArgumentParser
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


def sender(mailbox: Mailbox):
    this_actor.info("Send at full bandwidth")

    # First send a 2.5e8 Bytes payload at full bandwidth (1.25e8 Bps)
    payload = Engine.clock
    mailbox.put(payload, int(2.5e8))

    this_actor.info("Throttle the bandwidth at the Comm level")
    # ... then send it again but throttle the Comm
    payload = Engine.clock
    # get a handler on the comm first
    comm: Comm = mailbox.put_init(payload, int(2.5e8))

    # let throttle the communication. It amounts to set the rate of the comm to half the nominal bandwidth of the link,
    # i.e., 1.25e8 / 2. This second communication will thus take approximately twice as long as the first one
    comm.set_rate(int(1.25e8 / 2)).wait()


def receiver(mailbox: Mailbox):
    # Receive the first payload sent at full bandwidth
    sender_time = mailbox.get()
    communication_time = Engine.clock - sender_time
    this_actor.info(f"Payload received (full bandwidth) in {communication_time} seconds")

    # ... Then receive the second payload sent with a throttled Comm
    sender_time = mailbox.get()
    communication_time = Engine.clock - sender_time
    this_actor.info(f"Payload received (throttled) in {communication_time} seconds")


def main():
    settings = create_parser().parse_known_args()[0]
    e = Engine(sys.argv)
    e.load_platform(settings.platform)

    mailbox = e.mailbox_by_name_or_create("Mailbox")

    Actor.create("sender", e.host_by_name("node-0.simgrid.org"), sender, mailbox)
    Actor.create("receiver", e.host_by_name("node-1.simgrid.org"), receiver, mailbox)

    e.run()


if __name__ == "__main__":
    main()
