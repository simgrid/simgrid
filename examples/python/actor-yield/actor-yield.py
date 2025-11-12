# Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example does not much: It just spans over-polite actor that yield a large amount
of time before ending.

This serves as an example for the simgrid.yield() function, with which an actor can request
to be rescheduled after the other actor that are ready at the current timestamp.

It can also be used to benchmark our context-switching mechanism.
"""

import sys
from simgrid import Actor, Engine, Host, this_actor

def yielder(number_of_yields):
    for _ in range(number_of_yields):
        this_actor.yield_()
    this_actor.info("I yielded {:d} times. Goodbye now!".format(number_of_yields))

if __name__ == '__main__':
    e = Engine(sys.argv)

    e.load_platform(sys.argv[1])             # Load the platform description

    Host.by_name("Tremblay").add_actor("yielder", yielder, 10)
    Host.by_name("Ruby").add_actor("yielder", yielder, 15)

    e.run()  # - Run the simulation
