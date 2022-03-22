# Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package. 

"""
 This is a version of the masterworkers that (hopefully) survives to the chaos monkey.
 It tests synchronous send/receive as well as synchronous computations.

 It is not written to be pleasant to read, but instead to resist the aggressions of the monkey:
 - Workers keep going until after a global variable `todo` reaches 0.
 - The master is a daemon that just sends infinitely tasks
   (simgrid simulations stop as soon as all non-daemon actors are done).
 - The platform is created programmatically to remove path issues and control the problem size.

 See the simgrid-monkey script for more information.

 Inline configuration items:
 - host-count: how many actors to start (including the master
 - task-count: initial value of the `todo` global
 - deadline: time at which the simulation is known to be failed (to detect infinite loops).

"""

# Configuration items:
host_count = 3 # Host count (master on one, workers on the others)
task_count = 1 # Amount of tasks that must be executed to succeed
deadline = 120 # When to fail the simulation (infinite loop detection)
# End of configuration

import sys
from simgrid import Actor, Engine, Host, this_actor, Mailbox, NetZone, LinkInRoute, TimeoutException, NetworkFailureException

todo = task_count # remaining amount of tasks to execute, a global variable

def master():
  comp_size = int(1e6)
  comm_size = int(1e6)
  this_actor.info("Master booting")
  Actor.self().daemonize()
  this_actor.on_exit(lambda killed: this_actor.info("Master dying forcefully." if killed else "Master dying peacefully."))

  while True: # This is a daemon
    assert Engine.clock < deadline, f"Failed to run all tasks in less than {deadline} seconds. Is this an infinite loop?"

    try: 
      this_actor.info("Try to send a message")
      mailbox.put(comp_size, comm_size, 10.)
    except TimeoutException:
      this_actor.info("Timeouted while sending a task")
    except NetworkFailureException:
      this_actor.info("Got a NetworkFailureException. Wait a second before starting again.")
      this_actor.sleep_for(1.)

  assert False, "The impossible just happened (yet again): daemons shall not finish."

def worker(my_id):
  global todo
  this_actor.info(f"Worker {my_id} booting")
  this_actor.on_exit(lambda killed: this_actor.info(f"Worker {my_id} dying {'forcefully' if killed else 'peacefully'}."))

  while todo > 0:
    assert Engine.clock < deadline, f"Failed to run all tasks in less than {deadline} seconds. Is this an infinite loop?"

    try:
      this_actor.info(f"Waiting a message on mailbox")
      compute_cost = mailbox.get()

      this_actor.info("Start execution...")
      this_actor.execute(compute_cost)
      todo = todo - 1
      this_actor.info(f"Execution complete. Still {todo} to go.")

    except NetworkFailureException:
      this_actor.info("Got a NetworkFailureException. Wait a second before starting again.")
      this_actor.sleep_for(1.)
    except TimeoutException:
      this_actor.info("Timeouted while getting a task.")

if __name__ == '__main__':
  e = Engine(sys.argv)

  assert host_count > 2, "You need at least 2 workers (i.e., 3 hosts) or the master will be auto-killed when the only worker gets killed."
  assert todo > 0, "Please give some tasks to do to the workers."

  mailbox = Mailbox.by_name("mailbox")

  rootzone = NetZone.create_full_zone("Zone1")
  main = rootzone.create_host("lilibeth 0", 1e9)
  Actor.create("master", main, master).set_auto_restart(True)

  for i in range(1, host_count):
    link = rootzone.create_split_duplex_link(f"link {i}", "1MBps").set_latency("24us")
    host = rootzone.create_host(f"lilibeth {i}", 1e9)
    rootzone.add_route(main.netpoint, host.netpoint, None, None, [LinkInRoute(link, LinkInRoute.Direction.UP)], True)
    Actor.create("worker", host, worker, i).set_auto_restart(True)

  e.netzone_root.seal()
  e.run()

  this_actor.info("WE SURVIVED!")
