# Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

import sys

from simgrid import Engine, Actor, Comm, NetZone, Link, LinkInRoute, Mailbox, this_actor, NetworkFailureException


def sender(mailbox1_name: str, mailbox2_name: str) -> None:
    mailbox1: Mailbox = Mailbox.by_name(mailbox1_name)
    mailbox2: Mailbox = Mailbox.by_name(mailbox2_name)

    this_actor.info(f"Initiating asynchronous send to {mailbox1.name}")
    comm1: Comm = mailbox1.put_async(666, 5)

    this_actor.info(f"Initiating asynchronous send to {mailbox2.name}")
    comm2: Comm = mailbox2.put_async(666, 2)

    this_actor.info(f"Calling wait_any..")
    pending_comms = [comm1, comm2]
    try:
        index = Comm.wait_any([comm1, comm2])
        this_actor.info(f"Wait any returned index {index} (comm to {pending_comms[index].mailbox.name})")
    except NetworkFailureException:
        this_actor.info(f"Sender has experienced a network failure exception, so it knows that something went wrong")
        this_actor.info(f"Now it needs to figure out which of the two comms failed by looking at their state")

    this_actor.info(f"Comm to {comm1.mailbox.name} has state: {comm1.state_str}")
    this_actor.info(f"Comm to {comm2.mailbox.name} has state: {comm2.state_str}")

    try:
        comm1.wait()
    except NetworkFailureException:
        this_actor.info(f"Waiting on a FAILED comm raises an exception")

    this_actor.info("Wait for remaining comm, just to be nice")
    pending_comms.pop(0)
    try:
        Comm.wait_any(pending_comms)
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
    this_actor.info("sleeping 10 seconds...")
    this_actor.sleep_for(10.0)
    this_actor.info(f"turning off link {link_to_kill.name}")
    link_to_kill.turn_off()
    this_actor.info("link killed. exiting")


def main():
    e = Engine(sys.argv)
    zone: NetZone = NetZone.create_full_zone("AS0")
    host1 = zone.create_host("Host1", "1f")
    host2 = zone.create_host("Host2", "1f")
    host3 = zone.create_host("Host3", "1f")

    link_to_2 = LinkInRoute(zone.create_link("link_to_2", "1bps").seal())
    link_to_3 = LinkInRoute(zone.create_link("link_to_3", "1bps").seal())

    zone.add_route(host1.netpoint, host2.netpoint, None, None, [link_to_2], False)
    zone.add_route(host1.netpoint, host3.netpoint, None, None, [link_to_3], False)
    zone.seal()

    Actor.create("Sender", host1, sender, "mailbox2", "mailbox3")
    Actor.create("Receiver-1", host2, receiver, "mailbox2").daemonize()
    Actor.create("Receiver-2", host3, receiver, "mailbox3").daemonize()
    Actor.create("LinkKiller", host2, link_killer, "link_to_2").daemonize()

    e.run()


if __name__ == "__main__":
    main()
