# Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

# This example shows how to simulate a non-linear resource sharing for
# network links.

from simgrid import Actor, Engine, Comm, Mailbox, NetZone, Link, LinkInRoute, this_actor
import sys
import functools

class Sender:
  """
  Send a series of messages to mailbox "receiver"
  """
  def __init__(self, msg_count: int, msg_size=int(1e6)):
    self.msg_count = msg_count
    self.msg_size = msg_size

  # Actors that are created as object will execute their __call__ method.
  # So, the following constitutes the main function of the Sender actor.
  def __call__(self):
    pending_comms = []
    mbox = Mailbox.by_name("receiver")

    for i in range(self.msg_count):
      msg = "Message " + str(i)
      size = self.msg_size * (i + 1)
      this_actor.info("Send '%s' to '%s, msg size: %d'" % (msg, mbox.name, size))
      comm = mbox.put_async(msg, size)
      pending_comms.append(comm)

    this_actor.info("Done dispatching all messages")

    # Now that all message exchanges were initiated, wait for their completion in one single call
    Comm.wait_all(pending_comms)

    this_actor.info("Goodbye now!")

class Receiver:
  """
  Receiver actor: wait for N messages on the mailbox "receiver"
  """

  def __init__(self, msg_count=10):
    self.msg_count = msg_count

  def __call__(self):
    mbox = Mailbox.by_name("receiver")

    pending_msgs = []
    pending_comms = []

    this_actor.info("Wait for %d messages asynchronously" % self.msg_count)
    for _ in range(self.msg_count):
      comm, data = mbox.get_async()
      pending_comms.append(comm)
      pending_msgs.append(data)

    while len(pending_comms) > 0:
      index = Comm.wait_any(pending_comms)
      msg = pending_msgs[index].get()
      this_actor.info("I got '%s'." % msg)
      del pending_comms[index]
      del pending_msgs[index]

####################################################################################################
def link_nonlinear(link: Link, capacity: float, n: int) -> float:
  """
  Non-linear resource sharing for links

  Note that the callback is called twice in this example:
   1) link UP: with the number of active flows (from 9 to 1)
   2) link DOWN: with 0 active flows. A crosstraffic communication is happing
   in the down link, but it's not considered as an active flow.
  """
  # emulates a degradation in link according to the number of flows
  # you probably want something more complex than that and based on real
  # experiments
  capacity = min(capacity, capacity * (1.0 - (n - 1) / 10.0))
  this_actor.info("Link %s, %d active communications, new capacity %f" % (link.name, n, capacity))
  return capacity

def load_platform():
  """
  Create a simple 2-hosts platform */
   ________                 __________
  | Sender |===============| Receiver |
  |________|    Link1      |__________|

  """
  zone = NetZone.create_full_zone("Zone1")
  sender = zone.create_host("sender", 1).seal()
  receiver = zone.create_host("receiver", 1).seal()

  link = zone.create_split_duplex_link("link1", 1e6)
  # setting same callbacks (could be different) for link UP/DOWN in split-duplex link
  link.get_link_up().set_sharing_policy(
      Link.SharingPolicy.NONLINEAR,
      functools.partial(link_nonlinear, link.get_link_up()))
  link.get_link_down().set_sharing_policy(
      Link.SharingPolicy.NONLINEAR,
      functools.partial(link_nonlinear, link.get_link_down()))
  link.set_latency(10e-6).seal()

  # create routes between nodes
  zone.add_route(sender.get_netpoint(), receiver.get_netpoint(), None, None,
                 [LinkInRoute(link, LinkInRoute.Direction.UP)], True)
  zone.seal()

  # create actors Sender/Receiver
  Actor.create("receiver", receiver, Receiver(9))
  Actor.create("sender", sender, Sender(9))

###################################################################################################

if __name__ == '__main__':
  e = Engine(sys.argv)

  # create platform
  load_platform()

  # runs the simulation
  e.run()

  # explicitly deleting Engine object to avoid segfault during cleanup phase.
  # During Engine destruction, the cleanup of std::function linked to link_non_linear callback is called.
  # If we let the cleanup by itself, it fails trying on its destruction because the python main program
  # has already freed its variables
  del(e)
