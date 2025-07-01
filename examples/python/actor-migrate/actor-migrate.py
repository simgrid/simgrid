# Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example demonstrate the actor migrations.

The worker actor first move by itself, and then start an execution.
During that execution, the monitor migrates the worker, that wakes up on another host.
The execution was of the right amount of flops to take exactly 5 seconds on the first host
and 5 other seconds on the second one, so it stops after 10 seconds.

Then another migration is done by the monitor while the worker is suspended.

Note that worker() takes an uncommon set of parameters,
and that this is perfectly accepted by create().
"""

import sys
from simgrid import Actor, Engine, Host, this_actor


def worker(first_host, second_host):
    flop_amount = first_host.speed * 5 + second_host.speed * 5

    this_actor.info("Let's move to {:s} to execute {:.2f} Mflops (5sec on {:s} and 5sec on {:s})".format(
        first_host.name, flop_amount / 1e6, first_host.name, second_host.name))

    this_actor.set_host(first_host)
    this_actor.execute(flop_amount)

    this_actor.info("I wake up on {:s}. Let's suspend a bit".format(
        this_actor.get_host().name))

    this_actor.suspend()

    this_actor.info("I wake up on {:s}".format(this_actor.get_host().name))
    this_actor.info("Done")


def monitor():
    boivin = Host.by_name("Boivin")
    jacquelin = Host.by_name("Jacquelin")
    fafard = Host.by_name("Fafard")

    actor = fafard.add_actor("worker",worker, boivin, jacquelin)

    this_actor.sleep_for(5)

    this_actor.info(
        "After 5 seconds, move the actor to {:s}".format(jacquelin.name))
    actor.host = jacquelin

    this_actor.sleep_until(15)
    this_actor.info(
        "At t=15, move the actor to {:s} and resume it.".format(fafard.name))
    actor.host = fafard
    actor.resume()


if __name__ == '__main__':
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError(
            "Usage: actor-migration.py platform_file [other parameters]")
    e.load_platform(sys.argv[1])

    Host.by_name("Boivin").add_actor("monitor", monitor)
    e.run()
