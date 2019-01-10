# Copyright (c) 2017-2019. The SimGrid Team. All rights reserved.
#
# This program is free software you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

from simgrid import *
import sys


def lazy_guy():
    """The Lazy guy only wants to sleep, but can be awaken by the dream_master process"""
    this_actor.info("Nobody's watching me ? Let's go to sleep.")
    this_actor.suspend()  # - Start by suspending itself
    this_actor.info("Uuuh ? Did somebody call me ?")

    # - Then repetitively go to sleep, but get awaken
    this_actor.info("Going to sleep...")
    this_actor.sleep_for(10)
    this_actor.info("Mmm... waking up.")

    this_actor.info("Going to sleep one more time (for 10 sec)...")
    this_actor.sleep_for(10)
    this_actor.info("Waking up once for all!")

    this_actor.info("Ok, let's do some work, then (for 10 sec on Boivin).")
    this_actor.execute(980.95e6)

    this_actor.info("Mmmh, I'm done now. Goodbye.")


def dream_master():
    """The Dream master"""
    this_actor.info("Let's create a lazy guy.")  # Create a lazy_guy process
    lazy = Actor.create("Lazy", this_actor.get_host(), lazy_guy)
    this_actor.info("Let's wait a little bit...")
    this_actor.sleep_for(10)  # Wait for 10 seconds
    this_actor.info("Let's wake the lazy guy up! >:) BOOOOOUUUHHH!!!!")
    if lazy.is_suspended():
        lazy.resume()  # Then wake up the lazy_guy
    else:
        this_actor.error(
            "I was thinking that the lazy guy would be suspended now")

    this_actor.sleep_for(5)  # Repeat two times:
    this_actor.info("Suspend the lazy guy while he's sleeping...")
    lazy.suspend()  # Suspend the lazy_guy while he's asleep
    this_actor.info("Let him finish his siesta.")
    this_actor.sleep_for(10)  # Wait for 10 seconds
    this_actor.info("Wake up, lazy guy!")
    lazy.resume()  # Then wake up the lazy_guy again

    this_actor.sleep_for(5)
    this_actor.info("Suspend again the lazy guy while he's sleeping...")
    lazy.suspend()
    this_actor.info("This time, don't let him finish his siesta.")
    this_actor.sleep_for(2)
    this_actor.info("Wake up, lazy guy!")
    lazy.resume()

    this_actor.sleep_for(5)
    this_actor.info(
        "Give a 2 seconds break to the lazy guy while he's working...")
    lazy.suspend()
    this_actor.sleep_for(2)
    this_actor.info("Back to work, lazy guy!")
    lazy.resume()

    this_actor.info("OK, I'm done here.")


if __name__ == '__main__':
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError(
            "Usage: actor-suspend.py platform_file [other parameters]")

    e.load_platform(sys.argv[1])  # Load the platform description
    hosts = e.get_all_hosts()
    Actor.create("dream_master", hosts[0], dream_master)

    e.run()  # Run the simulation
