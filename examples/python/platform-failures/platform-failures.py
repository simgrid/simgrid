## Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.
 
from simgrid import Actor, Engine, Host, Mailbox, this_actor, NetworkFailureException, TimeoutException
import sys

# This example shows how to work with the state profile of a host or a link,
# specifying when the resource must be turned on or off.
#
# To set such a profile, the first way is to use a file in the XML, while the second is to use the programmatic
# interface, as exemplified in the main() below. Once this profile is in place, the resource will automatically
# be turned on and off.
#
# The actors running on a host that is turned off are forcefully killed
# once their on_exit callbacks are executed. They cannot avoid this fate.
# Since we specified on_failure="RESTART" for each actors in the XML file,
# they will be automatically restarted when the host starts again.
#
# Communications using failed links will .. fail.

def master(* args):
  assert len(args) == 4, f"Actor master requires 4 parameters, but got {len(args)} ones."
  tasks_count   = int(args[0])
  comp_size     = int(args[1])
  comm_size     = int(args[2])
  workers_count = int(args[3])

  this_actor.info(f"Got {workers_count} workers and {tasks_count} tasks to process")

  for i in range(tasks_count): # For each task to be executed: 
    # - Select a worker in a round-robin way
    mailbox = Mailbox.by_name(f"worker-{i % workers_count}")
    try:
      this_actor.info(f"Send a message to {mailbox.name}")
      mailbox.put(comp_size, comm_size, 10.0)
      this_actor.info(f"Send to {mailbox.name} completed")
    except TimeoutException:
      this_actor.info(f"Mmh. Got timeouted while speaking to '{mailbox.name}'. Nevermind. Let's keep going!")
    except NetworkFailureException:
      this_actor.info(f"Mmh. The communication with '{mailbox.name}' failed. Nevermind. Let's keep going!")

  this_actor.info("All tasks have been dispatched. Let's tell everybody the computation is over.")
  for i in range (workers_count):
    # - Eventually tell all the workers to stop by sending a "finalize" task
    mailbox = Mailbox.by_name(f"worker-{i % workers_count}")
    try:
      mailbox.put(-1.0, 0, 1.0)
    except TimeoutException:
      this_actor.info(f"Mmh. Got timeouted while speaking to '{mailbox.name}'. Nevermind. Let's keep going!")
    except NetworkFailureException:
      this_actor.info(f"Mmh. The communication with '{mailbox.name}' failed. Nevermind. Let's keep going!")

  this_actor.info("Goodbye now!")

def worker(* args):
  assert len(args) == 1, "Expecting one parameter"
  my_id               = int(args[0])

  mailbox = Mailbox.by_name(f"worker-{my_id}")
  done = False
  while not done:
    try:
      this_actor.info(f"Waiting a message on {mailbox.name}")
      compute_cost = mailbox.get()
      if compute_cost > 0: # If compute_cost is valid, execute a computation of that cost 
        this_actor.info("Start execution...")
        this_actor.execute(compute_cost)
        this_actor.info("Execution complete.")
      else: # Stop when receiving an invalid compute_cost
        this_actor.info("I'm done. See you!")
        done = True
    except NetworkFailureException:
      this_actor.info("Mmh. Something went wrong. Nevermind. Let's keep going!")

def sleeper():
  this_actor.info("Start sleeping...")
  this_actor.sleep_for(1)
  this_actor.info("done sleeping.")

if __name__ == '__main__':
  assert len(sys.argv) > 2, f"Usage: python app-masterworkers.py platform_file deployment_file"

  e = Engine(sys.argv)

  # This is how to attach a profile to an host that is created from the XML file.
  # This should be done before calling load_platform(), as the on_creation() event is fired when loading the platform.
  # You can never set a new profile to a resource that already have one.
  def on_creation(host):
    if (host.name == "Bourrassa"):
      host.set_state_profile("67 0\n70 1\n", 0)
  Host.on_creation_cb(on_creation)

  e.load_platform(sys.argv[1])

  e.register_actor("master", master)
  e.register_actor("worker", worker)
  e.load_deployment(sys.argv[2])

  # Add a new host programatically, and attach a state profile to it
  lili = e.netzone_root.create_host("Lilibeth", 1e15)
  lili.set_state_profile("4 0\n5 1\n", 10)
  lili.seal()

  # Create an actor on that new host, to monitor its own state
  actor = Actor.create("sleeper", lili, sleeper)
  actor.set_auto_restart(True)

  e.run()

  this_actor.info(f"Simulation time {e.clock():.4f}")
