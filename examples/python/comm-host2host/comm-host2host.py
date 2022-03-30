# Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This simple example demonstrates the Comm.sento_init() Comm.sento_async() functions,
that can be used to create a direct communication from one host to another without
relying on the mailbox mechanism.

There is not much to say, actually: The _init variant creates the communication and
leaves it unstarted (in case you want to modify this communication before it starts),
while the _async variant creates and start it. In both cases, you need to wait() it.

It is mostly useful when you want to have a centralized simulation of your settings,
with a central actor declaring all communications occurring on your distributed system.
"""

from argparse import ArgumentParser
import sys

from simgrid import Actor, Comm, Engine, Host, this_actor


def create_parser() -> ArgumentParser:
    parser = ArgumentParser()
    parser.add_argument(
        '--platform',
        type=str,
        required=True,
        help='path to the platform description'
    )
    return parser


def sender(h1: Host, h2: Host, h3: Host, h4: Host):
    this_actor.info(f"Send c12 with sendto_async({h1.name} -> {h2.name}),"
                    f" and c34 with sendto_init({h3.name} -> {h4.name})")
    c12: Comm = Comm.sendto_async(h1, h2, int(1.5e7))
    c34: Comm = Comm.sendto_init(h3, h4)
    c34.set_payload_size(int(1e7))

    # You can also detach() communications that you never plan to test() or wait().
    # Here we create a communication that only slows down the other ones
    noise: Comm = Comm.sendto_init(h1, h2)
    noise.set_payload_size(10000)
    noise.detach()

    this_actor.info(f"After creation, c12 is {c12.state_str} (remaining: {c12.remaining:.2e} bytes);"
                    f" c34 is {c34.state_str} (remaining: {c34.remaining:.2e} bytes)")
    this_actor.sleep_for(1)
    this_actor.info(f"One sec later, c12 is {c12.state_str} (remaining: {c12.remaining:.2e} bytes);"
                    f" c34 is {c34.state_str} (remaining: {c34.remaining:.2e} bytes)")
    c34.start()
    this_actor.info(f"After c34.start(), c12 is {c12.state_str} (remaining: {c12.remaining:.2e} bytes);"
                    f" c34 is {c34.state_str} (remaining: {c34.remaining:.2e} bytes)")
    c12.wait()
    this_actor.info(f"After c12.wait(), c12 is {c12.state_str} (remaining: {c12.remaining:.2e} bytes);"
                    f" c34 is {c34.state_str} (remaining: {c34.remaining:.2e} bytes)")
    c34.wait()
    this_actor.info(f"After c34.wait(), c12 is {c12.state_str} (remaining: {c12.remaining:.2e} bytes);"
                    f" c34 is {c34.state_str} (remaining: {c34.remaining:.2e} bytes)")

    # As usual, you don't have to explicitly start communications that were just init()ed.
    # The wait() will start it automatically.
    c14: Comm = Comm.sendto_init(h1, h4)
    c14.set_payload_size(100).wait()


def main():
    settings = create_parser().parse_known_args()[0]
    e = Engine(sys.argv)
    e.load_platform(settings.platform)
    Actor.create(
        "sender", e.host_by_name("Boivin"), sender,
        e.host_by_name("Tremblay"),  # h1
        e.host_by_name("Jupiter"),  # h2
        e.host_by_name("Fafard"),  # h3
        e.host_by_name("Ginette")  # h4
    )
    e.run()
    this_actor.info(f"Total simulation time: {e.clock:.3f}")


if __name__ == "__main__":
    main()
