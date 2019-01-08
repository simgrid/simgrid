# Copyright (c) 2017-2019. The SimGrid Team. All rights reserved.
#
# This program is free software you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

from simgrid import *
import sys


def sleeper():
    this_actor.info("Sleeper started")
    this_actor.sleep_for(3)
    this_actor.info("I'm done. See you!")


def master():
    this_actor.info("Start sleeper")
    actor = Actor.create("sleeper from master", Host.current(), sleeper)
    this_actor.info("Join the sleeper (timeout 2)")
    actor.join(2)

    this_actor.info("Start sleeper")
    actor = Actor.create("sleeper from master", Host.current(), sleeper)
    this_actor.info("Join the sleeper (timeout 4)")
    actor.join(4)

    this_actor.info("Start sleeper")
    actor = Actor.create("sleeper from master", Host.current(), sleeper)
    this_actor.info("Join the sleeper (timeout 2)")
    actor.join(2)

    this_actor.info("Start sleeper")
    actor = Actor.create("sleeper from master", Host.current(), sleeper)
    this_actor.info("Waiting 4")
    this_actor.sleep_for(4)
    this_actor.info("Join the sleeper after its end (timeout 1)")
    actor.join(1)

    this_actor.info("Goodbye now!")

    this_actor.sleep_for(1)

    this_actor.info("Goodbye now!")


if __name__ == '__main__':
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError(
            "Usage: actor-join.py platform_file [other parameters]")

    e.load_platform(sys.argv[1])

    Actor.create("master", Host.by_name("Tremblay"), master)

    e.run()

    this_actor.info("Simulation time {}".format(Engine.get_clock()))
