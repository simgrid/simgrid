# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how to declare dependencies between executions: a child execution only starts once all its parents
are completed.

Usage: exec-dependent.py platform_file [other parameters]
"""

import sys
from simgrid import ActivitySet, Engine, Exec, this_actor


def worker():
    # Define an amount of work that should take 1 second to execute.
    computation_amount = this_actor.get_host().speed

    # Create a small DAG
    # + Two parents and a child
    # + First parent ends after 1 second and the Second parent after 2 seconds.
    first_parent = this_actor.exec_init(computation_amount)
    second_parent = this_actor.exec_init(2 * computation_amount)
    child = Exec.init().set_flops_amount(computation_amount)

    pending_execs = ActivitySet([first_parent, second_parent, child])

    # Name the activities (for logging purposes only)
    first_parent.name = "parent 1"
    second_parent.name = "parent 2"
    child.name = "child"

    # Create the dependencies by declaring 'child' as a successor of first_parent and second_parent
    first_parent.add_successor(child)
    second_parent.add_successor(child)

    # Start the activities.
    first_parent.start()
    second_parent.start()
    child.start()

    # Wait for the completion of all activities
    while not pending_execs.empty():
        completed_one = pending_execs.wait_any()
        if completed_one is not None:
            this_actor.info(f"Exec '{completed_one.name}' is complete")


def on_veto(activity):
    # First display the situation
    this_actor.info(f"Activity '{activity.name}' vetoed. "
                    f"Dependencies: {'solved' if activity.dependencies_solved else 'NOT solved'}; "
                    f"Ressources: {'assigned' if activity.is_assigned else 'NOT assigned'}")

    # In this simple case, we just assign the child task to a resource when its dependencies are solved
    if activity.dependencies_solved and not activity.is_assigned:
        this_actor.info(f"Activity {activity.name}'s dependencies are resolved. Let's assign it to Fafard.")
        activity.host = Engine.instance.host_by_name("Fafard")


if __name__ == '__main__':
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file [other parameters]")
    e.load_platform(sys.argv[1])

    e.host_by_name("Fafard").add_actor("worker", worker)

    Exec.on_veto_cb(on_veto)

    e.run()

    this_actor.info(f"Simulation time {e.clock:g}")
