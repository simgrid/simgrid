# Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.

# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
# ##################################################################################
# Take this tutorial online: https://simgrid.org/doc/latest/Tutorial_Algorithms.html
# ##################################################################################
"""

import sys
from simgrid import Engine, Mailbox, this_actor

# master-begin
def master(*args):
    assert len(args) > 3, f"Actor master requires 3 parameters plus the workers' names, but got {len(args)}"
    tasks_count = int(args[0])
    compute_cost = int(args[1])
    communicate_cost = int(args[2])
    workers = []
    for i in range(3, len(args)):
        workers.append(Mailbox.by_name(args[i]))
    this_actor.info(f"Got {len(workers)} workers and {tasks_count} tasks to process")

    for i in range(tasks_count): # For each task to be executed:
        # - Select a worker in a round-robin way
        mailbox = workers[i % len(workers)]

        # - Send the computation amount to the worker
        if (tasks_count < 10000 or (tasks_count < 100000 and i % 10000 == 0) or i % 100000 == 0):
            this_actor.info(f"Sending task {i} of {tasks_count} to mailbox '{mailbox.name}'")
        mailbox.put(compute_cost, communicate_cost)

    this_actor.info("All tasks have been dispatched. Request all workers to stop.")
    for mailbox in workers:
        # The workers stop when receiving a negative compute_cost
        mailbox.put(-1, 0)
# master-end

# worker-begin
def worker(*args):
    assert not args, "The worker expects to not get any argument"

    mailbox = Mailbox.by_name(this_actor.get_host().name)
    done = False
    while not done:
        compute_cost = mailbox.get()
        if compute_cost > 0: # If compute_cost is valid, execute a computation of that cost
            this_actor.execute(compute_cost)
        else: # Stop when receiving an invalid compute_cost
            done = True

    this_actor.info("Exiting now.")
# worker-end

# main-begin
if __name__ == '__main__':
    assert len(sys.argv) > 2, f"Usage: python app-masterworkers.py platform_file deployment_file"

    e = Engine(sys.argv)

    # Register the classes representing the actors
    e.register_actor("master", master)
    e.register_actor("worker", worker)

    # Load the platform description and then deploy the application
    e.load_platform(sys.argv[1])
    e.load_deployment(sys.argv[2])

    # Run the simulation
    e.run()

    this_actor.info("Simulation is over")
# main-end
