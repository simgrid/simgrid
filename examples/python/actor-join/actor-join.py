# Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.
#
# This program is free software you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
Usage: actor-join.py platform_file [other parameters]
"""

import sys
from simgrid import Actor, Engine, Host, this_actor


def sleeper():
    this_actor.info("Sleeper started")
    this_actor.sleep_for(3)
    this_actor.info("I'm done. See you!")


def master():
    this_actor.info("Start 1st sleeper")
    actor = Host.current().add_actor("1st sleeper from master", sleeper)
    this_actor.info("Join the 1st sleeper (timeout 2)")
    actor.join(2)

    this_actor.info("Start 2nd sleeper")
    actor = Host.current().add_actor("2nd sleeper from master", sleeper)
    this_actor.info("Join the 2nd sleeper (timeout 4)")
    actor.join(4)

    this_actor.info("Start 3rd sleeper")
    actor = Host.current().add_actor("3rd sleeper from master", sleeper)
    this_actor.info("Join the 3rd sleeper (timeout 2)")
    actor.join(2)

    this_actor.info("Start 4th sleeper")
    actor = Host.current().add_actor("4th sleeper from master", sleeper)
    this_actor.info("Waiting 4")
    this_actor.sleep_for(4)
    this_actor.info("Join the 4th sleeper after its end (timeout 1)")
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

    Host.by_name("Tremblay").add_actor("master", master)

    e.run()

    this_actor.info("Simulation time {}".format(e.clock))
