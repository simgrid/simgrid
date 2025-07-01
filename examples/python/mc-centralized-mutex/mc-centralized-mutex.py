# Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

# **************** Centralized Mutual Exclusion Algorithm ******************
# This example implements a centralized mutual exclusion algorithm.        
# There is no bug on it, it is just provided to test the state space       
# reduction of DPOR.                                                       
# **************************************************************************

import sys
from enum import Enum
from simgrid import Actor, Engine, Host, Semaphore, NetZone, Mailbox, this_actor, MC_assert

CS_PER_PROCESS = 1
PROCESS_AMOUNT = 2

MsgKind = Enum('MsgKind', ['GRANT', 'REQUEST', 'RELEASE', 'FINISH'])

class Message:
  kind           = MsgKind.GRANT
  return_mailbox = None

  def __init__(self, kind,  mbox):
    self.kind = kind
    self.return_mailbox = mbox

def coordinator():
  requests = []
  mbox = Mailbox.by_name("coordinator")

  critical_section_used = False # initially the critical section is idle
  active_clients = PROCESS_AMOUNT

  while active_clients > 0:
    this_actor.info(f"Still {active_clients} clients")
    m = mbox.get()
    if m.kind == MsgKind.REQUEST:
      if (critical_section_used): # need to push the request in the vector
        this_actor.info("CS already used. Queue the request")
        requests.append(m.return_mailbox)
      else: # can serve it immediately
        this_actor.info(f"CS idle. Grant immediately to {m.return_mailbox.name}")
        m.return_mailbox.put(Message(MsgKind.GRANT, mbox), 1000)
        critical_section_used = True
    elif m.kind == MsgKind.FINISH:
      active_clients -= 1
    else: # that's a release. Check if someone was waiting for the lock
      if (requests != []):
        this_actor.info(f"CS release. Grant to queued requests (queue size: {len(requests)})")
        req = requests.pop()
        req.put(Message(MsgKind.GRANT, mbox), 1000)
      else: # nobody wants it
        this_actor.info("CS release. resource now idle")
        critical_section_used = False
  
  this_actor.info("Received all releases, quit now")

def client():
  my_pid = this_actor.get_pid()
  my_mailbox = Mailbox.by_name(str(my_pid))

  # request the CS several times, sleeping a bit in between
  for i in range (CS_PER_PROCESS):
    this_actor.info(f"Ask the request for the {i}th time of {CS_PER_PROCESS} times")
    Mailbox.by_name("coordinator").put(Message(MsgKind.REQUEST, my_mailbox), 1000)
    # wait for the answer
    grant = my_mailbox.get()
    this_actor.info("got the answer. Sleep a bit and release it")
    this_actor.sleep_for(1)

    Mailbox.by_name("coordinator").put(Message(MsgKind.RELEASE, my_mailbox), 1000)
    this_actor.sleep_for(my_pid)
  
  this_actor.info("Got all the CS I wanted, quit now")
  Mailbox.by_name("coordinator").put(Message(MsgKind.FINISH, my_mailbox), 1)

if __name__ == '__main__':
    e = Engine(sys.argv)

    e.load_platform(sys.argv[1])

    e.host_by_name("Tremblay").add_actor("coordinator", coordinator)
    for i in range(PROCESS_AMOUNT):
      e.host_by_name("Fafard").add_actor("client", client)

    e.run()
