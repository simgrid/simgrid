# Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

from simgrid import Actor, Engine, Host, this_actor
import sys

# This example does not much: It just spans over-polite actor that yield a large amount
# of time before ending.
#
# This serves as an example for the simgrid.yield() function, with which an actor can request
# to be rescheduled after the other actor that are ready at the current timestamp.
#
# It can also be used to benchmark our context-switching mechanism.


def yielder (number_of_yields):
    for _ in range(number_of_yields):
        this_actor.yield_()
    this_actor.info("I yielded {:d} times. Goodbye now!".format(number_of_yields))

if __name__ == '__main__':
    e = Engine(sys.argv)

    e.load_platform(sys.argv[1])             # Load the platform description
  
    Actor.create("yielder", Host.by_name("Tremblay"), yielder, 10)
    Actor.create("yielder", Host.by_name("Ruby"), yielder, 15)

    e.run()  # - Run the simulation
