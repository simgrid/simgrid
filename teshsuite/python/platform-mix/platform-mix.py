# Copyright (c) 2006-2022. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

from simgrid import Actor, Engine, Host, Mailbox, NetZone, LinkInRoute, this_actor
import sys

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

def load_platform():
  """ Creates a mixed platform, using many methods available in the API
  """

  root = NetZone.create_floyd_zone("root")
  hosts = []
  # dijkstra
  dijkstra = NetZone.create_dijkstra_zone("dijkstra")
  msg_base = "Creating zone: "
  this_actor.info(msg_base + dijkstra.name)
  dijkstra.set_parent(root)
  host1 = dijkstra.create_host("host1", [1e9, 1e8]).set_core_count(2)
  hosts.append(host1)
  host1.create_disk("disk1", 1e5, 1e4).seal()
  host1.create_disk("disk2", "1MBps", "1Mbps").seal()
  host1.seal()
  host2 = dijkstra.create_host("host2", ["1Gf", "1Mf"]).seal()
  hosts.append(host2)
  link1 = dijkstra.create_link("link1_up", [1e9]).set_latency(1e-3).set_concurrency_limit(10).seal()
  link2 = dijkstra.create_link("link1_down", ["1GBps"]).set_latency("1ms").seal()
  dijkstra.add_route(host1.get_netpoint(), host2.get_netpoint(), None, None, [LinkInRoute(link1)], False)
  dijkstra.add_route(host2.get_netpoint(), host1.get_netpoint(), None, None, [LinkInRoute(link2)], False)
  dijkstra.seal()

  # vivaldi
  vivaldi = NetZone.create_vivaldi_zone("vivaldi")
  this_actor.info(msg_base + vivaldi.name)
  vivaldi.set_parent(root)
  host3 = vivaldi.create_host("host3", 1e9).set_coordinates("1 1 1").seal()
  host4 = vivaldi.create_host("host4", "1Gf").set_coordinates("2 2 2").seal()
  hosts.append(host3)
  hosts.append(host4)

  # empty
  empty = NetZone.create_empty_zone("empty")
  this_actor.info(msg_base + empty.name)
  empty.set_parent(root)
  host5 = empty.create_host("host5", 1e9)
  hosts.append(host5)
  empty.seal()

  # wifi
  wifi = NetZone.create_wifi_zone("wifi")
  this_actor.info(msg_base + wifi.name)
  wifi.set_parent(root)
  router = wifi.create_router("wifi_router")
  wifi.set_property("access_point", "wifi_router")
  host6 = wifi.create_host(
      "host6", ["100.0Mf", "50.0Mf", "20.0Mf"]).seal()
  hosts.append(host6)
  wifi_link = wifi.create_link("AP1", ["54Mbps", "36Mbps", "24Mbps"]).seal()
  wifi_link.set_host_wifi_rate(host6, 1)
  wifi.seal()

  # create routes between netzones
  link_a = vivaldi.create_link("linkA", 1e9).seal()
  link_b = vivaldi.create_link("linkB", "1GBps").seal()
  link_c = vivaldi.create_link("linkC", "1GBps").seal()
  root.add_route(dijkstra.get_netpoint(), vivaldi.get_netpoint(
  ), host1.get_netpoint(), host3.get_netpoint(), [LinkInRoute(link_a)], True)
  root.add_route(vivaldi.get_netpoint(), empty.get_netpoint(
  ), host3.get_netpoint(), host5.get_netpoint(), [LinkInRoute(link_b)], True)
  root.add_route(empty.get_netpoint(), wifi.get_netpoint(
  ), host5.get_netpoint(), router, [LinkInRoute(link_c)], True)

  # create actors Sender/Receiver
  Actor.create("sender", hosts[0], Sender(hosts))
  for host in hosts:
    Actor.create("receiver", host, Receiver())

###################################################################################################

if __name__ == '__main__':
  e = Engine(sys.argv)

  # create platform
  load_platform()

  # runs the simulation
  e.run()
