# Copyright (c) 2006-2025. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

import sys
from simgrid import Actor, Engine, Host, Mailbox, NetZone, LinkInRoute, this_actor

class Sender:
    """
    Send 1 message for each host
    """
    def __init__(self, hosts):
        self.hosts = hosts

    # Actors that are created as object will execute their __call__ method.
    # So, the following constitutes the main function of the Sender actor.
    def __call__(self):

        for host in self.hosts:
            mbox = Mailbox.by_name(host.name)
            msg = "Hello. I'm " + str(this_actor.get_host().name)
            size = int(1e6)
            this_actor.info("Sending msg to " + host.name)
            mbox.put(msg, size)

        this_actor.info("Done dispatching all messages. Goodbye!")

class Receiver:
    """
    Receiver actor: wait for 1 messages and do operations
    """

    def __call__(self):
        this_actor.execute(1e9)
        for disk in Host.current().get_disks():
            this_actor.info("Using disk " + disk.name)
            disk.read(10000)
            disk.write(10000)
        mbox = Mailbox.by_name(this_actor.get_host().name)
        msg = mbox.get()
        this_actor.info("I got '%s'." % msg)
        this_actor.info("Finished executing. Goodbye!")

def load_platform(e: Engine):
    """ Creates a mixed platform, using many methods available in the API
    """

    root = e.netzone_root.add_netzone_floyd("root")
    hosts = []
    # dijkstra
    dijkstra = root.add_netzone_dijkstra("dijkstra", True)
    msg_base = "Creating zone: "
    this_actor.info(msg_base + dijkstra.name)
    host1 = dijkstra.add_host("host1", [1e9, 1e8])
    host1.core_count = 2
    hosts.append(host1)
    host1.add_disk("disk1", 1e5, 1e4).seal()
    host1.add_disk("disk2", "1MBps", "1Mbps").seal()
    host1.seal()
    dijkstra.set_gateway(host1)
    host2 = dijkstra.add_host("host2", ["1Gf", "1Mf"]).seal()
    hosts.append(host2)
    link1 = dijkstra.add_link("link1_up", [1e9]).set_latency(1e-3).set_concurrency_limit(10).seal()
    link2 = dijkstra.add_link("link1_down", ["1GBps"]).set_latency("1ms").seal()
    dijkstra.add_route(host1, host2, [LinkInRoute(link1)], False)
    dijkstra.add_route(host2, host1, [LinkInRoute(link2)], False)
    dijkstra.seal()

    # vivaldi
    vivaldi = root.add_netzone_vivaldi("vivaldi")
    this_actor.info(msg_base + vivaldi.name)
    host3 = vivaldi.add_host("host3", 1e9).set_coordinates("1 1 1").seal()
    vivaldi.set_gateway(host3)
    host4 = vivaldi.add_host("host4", "1Gf").set_coordinates("2 2 2").seal()
    hosts.append(host3)
    hosts.append(host4)

    # empty
    empty = root.add_netzone_empty("empty")
    this_actor.info(msg_base + empty.name)
    host5 = empty.add_host("host5", 1e9)
    empty.set_gateway(host5)
    hosts.append(host5)
    empty.seal()

    # wifi
    wifi = root.add_netzone_wifi("wifi")
    this_actor.info(msg_base + wifi.name)
    router = wifi.add_router("wifi_router")
    wifi.set_gateway(router)
    wifi.set_property("access_point", "wifi_router")
    host6 = wifi.add_host(
        "host6", ["100.0Mf", "50.0Mf", "20.0Mf"]).seal()
    hosts.append(host6)
    wifi_link = wifi.add_link("AP1", ["54Mbps", "36Mbps", "24Mbps"]).seal()
    wifi_link.set_host_wifi_rate(host6, 1)
    wifi.seal()

    # create routes between netzones
    link_a = vivaldi.add_link("linkA", 1e9).seal()
    link_b = vivaldi.add_link("linkB", "1GBps").seal()
    link_c = vivaldi.add_link("linkC", "1GBps").seal()
    root.add_route(dijkstra, vivaldi, [link_a])
    root.add_route(vivaldi, empty, [link_b])
    root.add_route(empty, wifi, [link_c])

    # create actors Sender/Receiver
    hosts[0].add_actor("sender", Sender(hosts))
    for host in hosts:
        host.add_actor("receiver", Receiver())

###################################################################################################

if __name__ == '__main__':
    e = Engine(sys.argv)

    # create platform
    load_platform(e)

    # runs the simulation
    e.run()
