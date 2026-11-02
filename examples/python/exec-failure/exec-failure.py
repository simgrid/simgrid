# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This examples shows how to survive to host failure exceptions that occur when an host is turned off.

The actors do not get notified when the host on which they run is turned off: they are just terminated
in this case, and their ``on_exit()`` callback gets executed.

For remote executions on failing hosts however, any blocking operation such as ``exec`` or ``wait`` will
raise an exception that you can catch and react to, as illustrated in this example.

Usage: exec-failure.py [other parameters]
"""

import sys
from simgrid import Engine, HostFailureException, this_actor


def show_execs_state(pending_execs):
    this_actor.info("State of each exec:")
    for activity in pending_execs:
        this_actor.info(f"  Exec on {activity.host.name} has state: {activity.state_str}")


def dispatcher(hosts):
    pending_execs = []
    for host in hosts:
        this_actor.info(f"Initiating asynchronous exec on {host.name}")
        # Computing 20 flops on an host which speed is 1f takes 20 seconds (when it does not fail)
        activity = this_actor.exec_init(20)
        activity.host = host
        pending_execs.append(activity)
        activity.start()

    this_actor.info("---------------------------------")
    this_actor.info("Wait on the first exec, which host is turned off at t=10 by the another actor.")
    try:
        pending_execs[0].wait()
        raise AssertionError("This wait was not supposed to succeed.")
    except HostFailureException:
        this_actor.info("Dispatcher has experienced a host failure exception, so it knows that something went wrong.")
    show_execs_state(pending_execs)

    this_actor.info("---------------------------------")
    this_actor.info("Wait on the second exec, which host is turned off at t=12 by the state profile.")
    try:
        pending_execs[1].wait()
        raise AssertionError("This wait was not supposed to succeed.")
    except HostFailureException:
        this_actor.info("Dispatcher has experienced a host failure exception, so it knows that something went wrong.")
    show_execs_state(pending_execs)

    this_actor.info("---------------------------------")
    this_actor.info("Wait on the third exec, which should succeed.")
    pending_execs[2].wait()  # A HostFailureException would be a bug here, so we don't catch it
    this_actor.info("No exception occurred.")
    show_execs_state(pending_execs)


def host_killer(to_kill):
    this_actor.sleep_for(10.0)
    this_actor.info(f"HostKiller turns off the host '{to_kill.name}'.")
    to_kill.turn_off()


if __name__ == '__main__':
    e = Engine(sys.argv)

    zone = e.netzone_root
    hosts = [zone.add_host(name, "1f") for name in ["Host1", "Host2", "Host3"]]

    # Attaching a state profile (ie a list of events changing the on/off state of the resource) to Host2.
    # The syntax of the profile (first parameter) is a list of "date state" lines.
    # So here, the host is turned off at t=12 and back on at t=20.
    # The last parameter is the period of that profile, meaning that it loops after 30 seconds.
    hosts[1].set_state_profile("""
12 0
20 1
""", 30)

    zone.seal()

    hosts[2].add_actor("Dispatcher", dispatcher, hosts)
    hosts[2].add_actor("HostKiller", host_killer, hosts[0])

    e.run()
