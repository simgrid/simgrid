# Copyright (c) 2018-2022. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

import sys
from simgrid import Actor, Engine, Host, this_actor


def executor():
    # execute() tells SimGrid to pause the calling actor until
    # its host has computed the amount of flops passed as a parameter
    this_actor.execute(98095)
    this_actor.info("Done.")
    # This simple example does not do anything beyond that


def privileged():
    # You can also specify the priority of your execution as follows.
    # An execution of priority 2 computes twice as fast as a regular one.
    #
    # So instead of a half/half sharing between the two executions,
    # we get a 1/3 vs 2/3 sharing.
    this_actor.execute(98095, priority=2)
    this_actor.info("Done.")

    # Note that the timings printed when executing this example are a bit misleading,
    # because the uneven sharing only last until the privileged actor ends.
    # After this point, the unprivileged one gets 100% of the CPU and finishes
    # quite quickly.


if __name__ == '__main__':
    e = Engine(sys.argv)
    e.load_platform(sys.argv[1])

    Actor.create("executor", Host.by_name("Tremblay"), executor)
    Actor.create("privileged", Host.by_name("Tremblay"), privileged)

    e.run()
