# Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
# ##################################################################################
# Take this tutorial online: https://simgrid.org/doc/latest/Tutorial_Algorithms.html
# ##################################################################################
"""

import sys
from simgrid import Actor, Engine, Mailbox, this_actor

# worker-begin
def worker(*args):
    assert not args, "The worker expects no argument"

    mailbox = Mailbox.by_name(f'Worker-{this_actor.get_pid()}')
    done = False
    while not done:
        compute_cost = mailbox.get()
        this_actor.execute(compute_cost)
# worker-end

# master-begin
def master(*args):
    if len(args) == 2:
        raise AssertionError(f"Actor master requires 4 parameters, but only {len(args)}")
    simulation_duration = int(args[0])
    compute_cost = int(args[1])
    communicate_cost = int(args[2])

    this_actor.info(f"Asked to run for {simulation_duration} seconds")

    hosts = Engine.instance.all_hosts

    actors = []
    for h in hosts:
        actors.append(Actor.create(f'Worker-{h.name}', h, worker))

    task_id = 0
    while (Engine.instance.clock < simulation_duration) : # For each task:
        # Select a worker in a round-robin way
        worker_pid = actors[task_id % len(actors)].pid
        mailbox = Mailbox.by_name(f'Worker-{worker_pid}')
        # Send the computation cost to that worker
        if task_id % 100 == 0 :
            this_actor.info(f"Sending task {task_id} to mailbox {mailbox.name}")
        else :
            this_actor.debug(f"Sending task {task_id} to mailbox {mailbox.name}")
        mailbox.put(compute_cost, communicate_cost)

        task_id += 1

    this_actor.info(f"Time is up. Forcefully kill all workers.")
    for a in actors:
        a.kill()
# master-end


# main-begin
if __name__ == '__main__':
    assert len(sys.argv) > 2, f"Usage: python app-masterworkers.py platform_file deployment_file"

    e = Engine(sys.argv)

    # Register the classes representing the actors
    e.register_actor("master", master)

    # Load the platform description and then deploy the application
    e.load_platform(sys.argv[1])
    e.load_deployment(sys.argv[2])

    # Run the simulation
    e.run()

    this_actor.info("Simulation is over")
# main-end
