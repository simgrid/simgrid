# Copyright (c) 2018-2019. The SimGrid Team. All rights reserved.
#
# This program is free software you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

import sys
from simgrid import *


class Waiter:
    """ This actor simply waits for its task completion after starting it. That's exactly equivalent to synchronous execution. """

    def __call__(self):
        computation_amount = this_actor.get_host().speed
        this_actor.info("Execute {:.0f} flops, should take 1 second.".format(computation_amount))
        activity = this_actor.exec_init(computation_amount)
        activity.start()
        activity.wait()

        this_actor.info("Goodbye now!")


class Monitor:
    """This actor tests the ongoing execution until its completion, and don't wait before it's terminated."""

    def __call__(self):
        computation_amount = this_actor.get_host().speed
        this_actor.info("Execute {:.0f} flops, should take 1 second.".format(computation_amount))
        activity = this_actor.exec_init(computation_amount).start()

        while not activity.test():
            this_actor.info("Remaining amount of flops: {:.0f} ({:.0f}%)".format(
                activity.remaining, 100 * activity.remaining_ratio))
            this_actor.sleep_for(0.3)
        activity.wait()

        this_actor.info("Goodbye now!")


class Canceller:
    """This actor cancels the ongoing execution after a while."""

    def __call__(self):
        computation_amount = this_actor.get_host().speed
        this_actor.info("Execute {:.0f} flops, should take 1 second.".format(computation_amount))
        activity = this_actor.exec_init(computation_amount).start()

        this_actor.sleep_for(0.5)
        this_actor.info("I changed my mind, cancel!")
        activity.cancel()

        this_actor.info("Goodbye now!")


if __name__ == '__main__':
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError("Usage: exec-async.py platform_file [other parameters]")

    e.load_platform(sys.argv[1])

    Actor.create("wait", Host.by_name("Fafard"), Waiter())
    Actor.create("monitor", Host.by_name("Ginette"), Monitor())
    Actor.create("cancel", Host.by_name("Boivin"), Canceller())

    e.run()
