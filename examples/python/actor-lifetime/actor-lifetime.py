# Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This Python file acts as the foil to the corresponding XML file, where the
action takes place: Actors are started and stopped at predefined time
"""

import sys
from simgrid import Engine, this_actor


class Sleeper:
    """This actor just sleeps until termination"""

    def __init__(self):
        this_actor.on_exit(lambda killed: this_actor.info("Exiting now (killed)." if killed else "Exiting now (finishing)."))

    def __call__(self):
        this_actor.info("Hello! I go to sleep.")
        this_actor.sleep_for(10)
        this_actor.info("Done sleeping.")


if __name__ == '__main__':
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError(
            "Usage: actor-lifetime.py platform_file actor-lifetime_d.xml [other parameters]")

    e.load_platform(sys.argv[1])     # Load the platform description
    e.register_actor("sleeper", Sleeper)
    # Deploy the sleeper actors with explicit start/kill times
    e.load_deployment(sys.argv[2])

    e.run()
