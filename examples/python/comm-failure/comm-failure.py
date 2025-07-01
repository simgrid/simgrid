# Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

import sys

from simgrid import Engine, Actor, ActivitySet, Comm, NetZone, Link, LinkInRoute, Mailbox, this_actor, NetworkFailureException


def sender(mailbox1_name: str, mailbox2_name: str) -> None:
    mailbox1: Mailbox = Mailbox.by_name(mailbox1_name)
    mailbox2: Mailbox = Mailbox.by_name(mailbox2_name)

    this_actor.info(f"Initiating asynchronous send to {mailbox1.name}")
    comm1: Comm = mailbox1.put_async(666, 5)

    this_actor.info(f"Initiating asynchronous send to {mailbox2.name}")
    comm2: Comm = mailbox2.put_async(666, 2)

    this_actor.info("Calling wait_any..")
    pending_comms = ActivitySet([comm1, comm2])
    try:
        comm = pending_comms.wait_any()
        this_actor.info(f"Wait any returned a comm to {comm.mailbox.name})")
    except NetworkFailureException:
        this_actor.info("Sender has experienced a network failure exception, so it knows that something went wrong")
        this_actor.info("Now it needs to figure out which of the two comms failed by looking at their state:")

    this_actor.info(f"  Comm to {comm1.mailbox.name} has state: {comm1.state_str}")
    this_actor.info(f"  Comm to {comm2.mailbox.name} has state: {comm2.state_str}")

    try:
        comm1.wait()
    except NetworkFailureException as err:
        this_actor.info(f"Waiting on a FAILED comm raises an exception: '{err}'")

    this_actor.info("Wait for remaining comm, just to be nice")
    try:
        pending_comms.wait_all()
    except Exception as e:
        this_actor.warning(str(e))


def receiver(mailbox_name: str) -> None:
    mailbox: Mailbox = Mailbox.by_name(mailbox_name)
    this_actor.info(f"Receiver posting a receive ({mailbox_name})...")
    try:
        mailbox.get()
        this_actor.info(f"Receiver has received successfully ({mailbox_name})!")
    except NetworkFailureException:
        this_actor.info(f"Receiver has experience a network failure exception ({mailbox_name})")


def link_killer(link_name: str) -> None:
    link_to_kill = Link.by_name(link_name)
    this_actor.sleep_for(10.0)
    this_actor.info(f"Turning off link '{link_to_kill.name}'")
    link_to_kill.turn_off()

def main():
    e = Engine(sys.argv)
    zone: NetZone = e.netzone_root
    host1 = zone.add_host("Host1", "1f")
    host2 = zone.add_host("Host2", "1f")
    host3 = zone.add_host("Host3", "1f")

    link_to_2 = zone.add_link("link_to_2", "1bps").seal()
    link_to_3 = zone.add_link("link_to_3", "1bps").seal()

    zone.add_route(host1, host2, [link_to_2])
    zone.add_route(host1, host3, [link_to_3])
    zone.seal()

    host1.add_actor("Sender", sender, "mailbox2", "mailbox3")
    host2.add_actor("Receiver-1", receiver, "mailbox2").daemonize()
    host3.add_actor("Receiver-2", receiver, "mailbox3").daemonize()
    host2.add_actor("LinkKiller", link_killer, "link_to_2").daemonize()

    e.run()


if __name__ == "__main__":
    main()
