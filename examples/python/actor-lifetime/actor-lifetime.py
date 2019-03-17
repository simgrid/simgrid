# Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

# This Python file acts as the foil to the corresponding XML file, where the
# action takes place: Actors are started and stopped at predefined time

from simgrid import *
import sys


class Sleeper:
    """This actor just sleeps until termination"""

    def __init__(self, *args):
        # sys.exit(1); simgrid.info("Exiting now (done sleeping or got killed)."))
        this_actor.on_exit(lambda: print("BAAA"))

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
    # Deploy the sleeper processes with explicit start/kill times
    e.load_deployment(sys.argv[2])

    e.run()
