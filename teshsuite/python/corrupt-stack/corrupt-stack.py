# Copyright (c) 2019-2021. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

# Second failing example for bug #9 on Framagit (Python bindings crashing)
#
# An intricate recursion is used to make the crash happen.

import sys
from simgrid import Engine, this_actor

def do_sleep1(i, dur):
    if i > 0:
        this_actor.info("1-Iter {:d}".format(i))
        do_sleep3(i - 1, dur)
        this_actor.sleep_for(dur)
        this_actor.info("1-Mid ({:d})".format(i))
        do_sleep3(int(i / 2), dur)
        this_actor.info("1-Done ({:d})".format(i))

def do_sleep3(i, dur):
    if i > 0:
        this_actor.info("3-Iter {:d}".format(i))
        do_sleep5(i - 1, dur)
        this_actor.sleep_for(dur)
        this_actor.info("3-Mid ({:d})".format(i))
        do_sleep5(int(i / 2), dur)
        this_actor.info("3-Done ({:d})".format(i))

def do_sleep5(i, dur):
    if i > 0:
        this_actor.info("5-Iter {:d}".format(i))
        do_sleep1(i - 1, dur)
        this_actor.sleep_for(dur)
        this_actor.info("5-Mid ({:d})".format(i))
        do_sleep1(int(i / 2), dur)
        this_actor.info("5-Done ({:d})".format(i))

def sleeper1():
    do_sleep1(16, 1)

def sleeper3():
    do_sleep3(6, 3)

def sleeper5():
    do_sleep5(4, 5)

if __name__ == '__main__':
    e = Engine(sys.argv)

    e.load_platform(sys.argv[1])             # Load the platform description

    # Register the classes representing the actors
    e.register_actor("sleeper1", sleeper1)
    e.register_actor("sleeper3", sleeper3)
    e.register_actor("sleeper5", sleeper5)

    e.load_deployment(sys.argv[2])

    e.run()
    this_actor.info("Finalize!")
