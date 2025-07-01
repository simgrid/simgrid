# Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.
#
# This program is free software you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
Usage: actor-daemon.py platform_file [other parameters]
"""

import sys
from simgrid import Actor, Engine, Host, this_actor


def worker():
    """The worker actor, working for a while before leaving"""
    this_actor.info("Let's do some work (for 10 sec on Boivin).")
    this_actor.execute(980.95e6)

    this_actor.info("I'm done now. I leave even if it makes the daemon die.")


def my_daemon():
    """The daemon, displaying a message every 3 seconds until all other actors stop"""
    Actor.self().daemonize()

    while True:
        this_actor.info("Hello from the infinite loop")
        this_actor.sleep_for(3.0)

    this_actor.info(
        "I will never reach that point: daemons are killed when regular actors are done")


if __name__ == '__main__':
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError(
            "Usage: actor-daemon.py platform_file [other parameters]")

    e.load_platform(sys.argv[1])
    Host.by_name("Boivin").add_actor("worker", worker)
    Host.by_name("Tremblay").add_actor("daemon", my_daemon)

    e.run()
