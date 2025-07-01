# Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
Usage: actor-kill.py platform_file [other parameters]
"""

import sys
from simgrid import Actor, Engine, Host, this_actor


def victim_a_fun():
    this_actor.on_exit(lambda forcefully: this_actor.info("I have been killed!" if forcefully else "I finish now."))
    this_actor.info("Hello!")
    this_actor.info("Suspending myself")
    this_actor.suspend()                        # - Start by suspending itself
    # - Then is resumed and start to execute a task
    this_actor.info("OK, OK. Let's work")
    this_actor.execute(1e9)
    # - But will never reach the end of it
    this_actor.info("Bye!")


def victim_b_fun():
    this_actor.info("Terminate before being killed")


def killer():
    this_actor.info("Hello!")  # - First start a victim actor
    victim_a = Host.by_name("Fafard").add_actor("victim A", victim_a_fun)
    victim_b = Host.by_name("Jupiter").add_actor("victim B", victim_b_fun)
    this_actor.sleep_for(10)  # - Wait for 10 seconds

    # - Resume it from its suspended state
    this_actor.info("Resume the victim A")
    victim_a.resume()
    this_actor.sleep_for(2)

    this_actor.info("Kill the victim A")   # - and then kill it
    Actor.by_pid(victim_a.pid).kill()       # You can retrieve an actor from its PID (and then kill it)

    this_actor.sleep_for(1)

    # that's a no-op, there is no zombies in SimGrid
    this_actor.info("Kill victim B, even if it's already dead")
    victim_b.kill()

    this_actor.sleep_for(1)

    this_actor.info("Start a new actor, and kill it right away")
    victim_c = Host.by_name("Jupiter").add_actor("victim C", victim_a_fun)
    victim_c.kill()

    this_actor.sleep_for(1)

    this_actor.info("Killing everybody but myself")
    Actor.kill_all()

    this_actor.info("OK, goodbye now. I commit a suicide.")
    this_actor.exit()

    this_actor.info(
        "This line never gets displayed: I'm already dead since the previous line.")


if __name__ == '__main__':
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError(
            "Usage: actor-kill.py platform_file [other parameters]")

    e.load_platform(sys.argv[1])     # Load the platform description
    # Create and deploy killer actor, that will create the victim actors
    Host.by_name("Tremblay").add_actor("killer",killer)

    e.run()
