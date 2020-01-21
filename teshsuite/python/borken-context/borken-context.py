# Copyright (c) 2019-2020. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

# First failing example for bug #9 on Framagit (Python bindings crashing)
#
# A heavy operation is added after e.run() (here, it's "import gc"). Note that
# the failure only occurs when the actor "sender" starts first, and the
# scheduling is interlaced: sender starts, receiver starts, sender terminates,
# receiver terminates.

import sys
from simgrid import *

def Sender():
    this_actor.sleep_for(3)
    this_actor.info("Goodbye now!")

def Receiver():
    this_actor.sleep_for(5)
    this_actor.info("Five seconds elapsed")

if __name__ == '__main__':
    e = Engine(sys.argv)

    e.load_platform(sys.argv[1])             # Load the platform description

    # Register the classes representing the actors
    e.register_actor("sender", Sender)
    e.register_actor("receiver", Receiver)

    e.load_deployment(sys.argv[2])

    e.run()
    this_actor.info("Dummy import...")
    import gc
    this_actor.info("done.")
