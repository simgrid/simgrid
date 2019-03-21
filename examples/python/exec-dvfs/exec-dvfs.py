# Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

import sys
from simgrid import *


class Dvfs:
    def __call__(self):
        workload = 100E6
        host = this_actor.get_host()

        nb = host.get_pstate_count()
        this_actor.info("Count of Processor states={:d}".format(nb))

        this_actor.info("Current power peak={:f}".format(host.speed))

        # Run a task
        this_actor.execute(workload)

        task_time = Engine.get_clock()
        this_actor.info("Task1 duration: {:.2f}".format(task_time))

        # Change power peak
        new_pstate = 2

        this_actor.info("Changing power peak value to {:f} (at index {:d})".format( host.get_pstate_speed(new_pstate), new_pstate))

        host.pstate = new_pstate

        this_actor.info("Current power peak={:f}".format(host.speed))

        # Run a second task
        this_actor.execute(workload)

        task_time = Engine.get_clock() - task_time
        this_actor.info("Task2 duration: {:.2f}".format(task_time))

        # Verify that the default pstate is set to 0
        host2 = Host.by_name("MyHost2")
        this_actor.info("Count of Processor states={:d}".format(host2.get_pstate_count()))

        this_actor.info("Current power peak={:f}".format(host2.speed))

if __name__ == '__main__':
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError("Usage: exec-dvfs.py platform_file [other parameters] (got {:d} params)".format(len(sys.argv)))

    e.load_platform(sys.argv[1])
    Actor.create("dvfs_test", Host.by_name("MyHost1"), Dvfs())
    Actor.create("dvfs_test", Host.by_name("MyHost2"), Dvfs())

    e.run()
