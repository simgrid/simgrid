# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example shows how to suspend and resume an asynchronous execution, and how to be notified when it happens.

Usage: exec-suspend.py platform_file [other parameters]
"""

import sys
from simgrid import Engine, this_actor


def suspend_activity_for(activity, seconds):
    activity.suspend()
    remaining_before_sleep = activity.remaining
    this_actor.sleep_for(seconds)
    activity.resume()

    # Sanity check: remaining computations have to be the same before and after this period
    this_actor.info(f"Activity hasn't advanced: {'yes' if remaining_before_sleep == activity.remaining else 'no'}. "
                    f"(remaining before sleep: {remaining_before_sleep:f}, "
                    f"remaining after sleep: {activity.remaining:f})")


def executor():
    computation_amount = this_actor.get_host().speed

    # Execution will take 3 seconds
    activity = this_actor.exec_init(3 * computation_amount)

    # Add functions to be called when the activity is resumed or suspended
    activity.on_this_suspend_cb(lambda exec_: this_actor.info(f"Task is suspended (remaining: {exec_.remaining:f})"))
    activity.on_this_resume_cb(lambda exec_: this_actor.info(f"Task is resumed (remaining: {exec_.remaining:f})"))

    # The first second of the execution
    activity.start()
    this_actor.sleep_for(1)

    # Suspend the activity for 10 seconds
    suspend_activity_for(activity, 10)

    # Run the activity for one second
    this_actor.sleep_for(1)

    # Suspend the activity for the second time
    suspend_activity_for(activity, 10)

    # Finish execution
    activity.wait()
    this_actor.info("Finished")


if __name__ == '__main__':
    e = Engine(sys.argv)
    if len(sys.argv) < 2:
        raise AssertionError(f"Usage: {sys.argv[0]} platform_file [other parameters]")
    e.load_platform(sys.argv[1])

    e.host_by_name("Tremblay").add_actor("executor", executor)

    e.run()
