# Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

from argparse import ArgumentParser
import sys

from simgrid import Engine, Actor, Mailbox, this_actor


def create_parser() -> ArgumentParser:
    parser = ArgumentParser()
    parser.add_argument(
        '--platform',
        type=str,
        required=True,
        help='path to the platform description'
    )
    return parser


def pinger(mailbox_in: Mailbox, mailbox_out: Mailbox):
    this_actor.info(f"Ping from mailbox {mailbox_in.name} to mailbox {mailbox_out.name}")

    # Do the ping with a 1-Byte payload (latency bound) ...
    payload = Engine.clock
    mailbox_out.put(payload, 1)

    # ... then wait for the (large) pong
    sender_time: float = mailbox_in.get()
    communication_time = Engine.clock - sender_time
    this_actor.info("Payload received : large communication (bandwidth bound)")
    this_actor.info(f"Pong time (bandwidth bound): {communication_time:.3f}")


def ponger(mailbox_in: Mailbox, mailbox_out: Mailbox):
    this_actor.info(f"Pong from mailbox {mailbox_in.name} to mailbox {mailbox_out.name}")

    # Receive the (small) ping first ...
    sender_time: float = mailbox_in.get()
    communication_time = Engine.clock - sender_time
    this_actor.info("Payload received : small communication (latency bound)")
    this_actor.info(f"Ping time (latency bound) {communication_time:.3f}")

    # ... Then send a 1GB pong back (bandwidth bound)
    payload = Engine.clock
    this_actor.info(f"payload = {payload:.3f}")
    mailbox_out.put(payload, int(1e9))


def main():
    settings = create_parser().parse_known_args()[0]
    e = Engine(sys.argv)
    e.load_platform(settings.platform)

    mb1: Mailbox = e.mailbox_by_name_or_create("Mailbox 1")
    mb2: Mailbox = e.mailbox_by_name_or_create("Mailbox 2")

    Actor.create("pinger", e.host_by_name("Tremblay"), pinger, mb1, mb2)
    Actor.create("ponger", e.host_by_name("Jupiter"), ponger, mb2, mb1)

    e.run()

    this_actor.info(f"Total simulation time: {e.clock:.3f}")


if __name__ == "__main__":
    main()
