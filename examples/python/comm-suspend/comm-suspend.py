# Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

""" This example shows how to suspend and resume an asynchronous communication.
"""

from argparse import ArgumentParser
import sys

from simgrid import Actor, Comm, Engine, Mailbox, this_actor


def create_parser() -> ArgumentParser:
    parser = ArgumentParser()
    parser.add_argument(
        '--platform',
        type=str,
        required=True,
        help='path to the platform description'
    )
    return parser


def sender():
    mailbox: Mailbox = Mailbox.by_name("receiver")
    payload = "Sent message"

    # Create a communication representing the ongoing communication and then
    simulated_size_in_bytes = 13194230
    comm: Comm = mailbox.put_init(payload, simulated_size_in_bytes)
    this_actor.info(f"Suspend the communication before it starts (remaining: {comm.remaining:.0f} bytes)"
                    f" and wait a second.")
    this_actor.sleep_for(1)
    this_actor.info(f"Now, start the communication (remaining: {comm.remaining:.0f} bytes) and wait another second.")
    comm.start()
    this_actor.sleep_for(1)
    this_actor.info(f"There is still {comm.remaining:.0f} bytes to transfer in this communication."
                    " Suspend it for one second.")
    comm.suspend()
    this_actor.info(f"Now there is {comm.remaining:.0f} bytes to transfer. Resume it and wait for its completion.")
    comm.resume()
    comm.wait()
    this_actor.info(f"There is {comm.remaining:.0f} bytes to transfer after the communication completion.")
    this_actor.info(f"Suspending a completed activity is a no-op.")
    comm.suspend()


def receiver():
    mailbox: Mailbox = Mailbox.by_name("receiver")
    this_actor.info("Wait for the message.")
    received: str = mailbox.get()
    this_actor.info(f"I got '{received}'.")


def main():
    settings = create_parser().parse_known_args()[0]
    e = Engine(sys.argv)
    e.load_platform(settings.platform)
    Actor.create("sender", e.host_by_name("Tremblay"), sender)
    Actor.create("receiver", e.host_by_name("Jupiter"), receiver)
    e.run()


if __name__ == "__main__":
    main()
