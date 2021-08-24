# Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

# This example shows how to simulate a non-linear resource sharing for
# CPUs.


from simgrid import Actor, Engine, NetZone, Host, this_actor
import sys
import functools


def runner():
    computation_amount = this_actor.get_host().speed
    n_task = 10

    this_actor.info("Execute %d tasks of %g flops, should take %d second in a CPU without degradation. It will take the double here." % (
        n_task, computation_amount, n_task))
    tasks = [this_actor.exec_init(computation_amount).start()
             for _ in range(n_task)]

    this_actor.info("Waiting for all tasks to be done!")
    for task in tasks:
        task.wait()

    this_actor.info("Finished executing. Goodbye now!")


def cpu_nonlinear(host: Host, capacity: float, n: int) -> float:
    """ Non-linear resource sharing for CPU """
    # emulates a degradation in CPU according to the number of tasks
    # totally unrealistic but for learning purposes
    capacity = capacity / 2 if n > 1 else capacity
    this_actor.info("Host %s, %d concurrent tasks, new capacity %f" %
                    (host.name, n, capacity))
    return capacity


def load_platform():
    """ Create a simple 1-host platform """
    zone = NetZone.create_empty_zone("Zone1")
    runner_host = zone.create_host("runner", 1e6)
    runner_host.set_sharing_policy(
        Host.SharingPolicy.NONLINEAR, functools.partial(cpu_nonlinear, runner_host))
    runner_host.seal()
    zone.seal()

    # create actor runner
    Actor.create("runner", runner_host, runner)


if __name__ == '__main__':
    e = Engine(sys.argv)

    # create platform
    load_platform()

    # runs the simulation
    e.run()

    # explicitly deleting Engine object to avoid segfault during cleanup phase.
    # During Engine destruction, the cleanup of std::function linked to non_linear callback is called.
    # If we let the cleanup by itself, it fails trying on its destruction because the python main program
    # has already freed its variables
    del(e)
