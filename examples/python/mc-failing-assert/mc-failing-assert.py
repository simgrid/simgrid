# Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

# ******************* Non-deterministic message ordering  ******************
# Server assumes a fixed order in the reception of messages from its clients
# which is incorrect because the message ordering is non-deterministic      
# **************************************************************************

import sys
from simgrid import Actor, Engine, Host, Semaphore, NetZone, Mailbox, this_actor, MC_assert

def server(worker_amount: int):
  value_got = -1
  mb = Mailbox.by_name("server")
  for _ in range(worker_amount):
    value_got = mb.get()

  # We assert here that the last message we got (which overwrite any previously received message) is the one from the
  # last worker This will obviously fail when the messages are received out of order.
  MC_assert(value_got == 2)

  this_actor.info("OK")
  return 0

def client(rank: int):
  # I just send my rank onto the mailbox. It must be passed as a stable memory block (thus the new) so that that
  # memory survives even after the end of the client */

  mailbox = Mailbox.by_name("server")
  mailbox.put(rank, 1) # communication cost (second parameter) is not really relevant in MC mode

  this_actor.info("Sent!")

if __name__ == '__main__':
    e = Engine(sys.argv)

    e.load_platform(sys.argv[1])             # Load the platform description

    hosts = e.all_hosts
    assert len(hosts) >= 3, "This example requires at least 3 hosts"

    hosts[0].add_actor("server", server, 2)
    hosts[1].add_actor("client1", client, 1)
    hosts[2].add_actor("client2", client, 2)

    e.run()
