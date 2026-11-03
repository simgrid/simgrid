# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how to compute the energy consumed by a wifi link.

Usage: energy-wifi.py platform_file [other parameters]
"""

import sys
from simgrid import Engine, Mailbox, sg_wifi_energy_plugin_init, this_actor


def sender():
    # start sending after 5 seconds
    this_actor.sleep_until(5)

    mb_name = "MailBoxRCV"
    dst = Mailbox.by_name(mb_name)

    size = 6750000

    this_actor.info(f"SENDING 1 msg of size {size} to {mb_name}")
    dst.put("message", size)
    this_actor.info("finished sending")


def receiver():
    mb_name = "MailBoxRCV"
    this_actor.info(f"RECEIVING on mb {mb_name}")
    my_box = Mailbox.by_name(mb_name)
    my_box.get()

    this_actor.info("received all messages")


if __name__ == '__main__':
    e = Engine(sys.argv)
    sg_wifi_energy_plugin_init()
    if len(sys.argv) < 2:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file [other parameters]")
    e.load_platform(sys.argv[1])

    # setup WiFi bandwidths
    link = e.link_by_name("AP1")
    link.set_host_wifi_rate(e.host_by_name("Station 1"), 0)
    link.set_host_wifi_rate(e.host_by_name("Station 2"), 0)

    # create the two actors for the test
    e.host_by_name("Station 1").add_actor("act0", sender)
    e.host_by_name("Station 2").add_actor("act1", receiver)

    e.run()
