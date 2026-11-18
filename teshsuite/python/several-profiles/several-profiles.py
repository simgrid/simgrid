# Copyright (c) 2026. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
Attach profiles of every kind to several resources from Python.

Each profile set from Python used to be registered under the same empty name, so the second one aborted the
simulation with "Refusing to define trace  twice".
"""

import sys
from simgrid import Engine, this_actor


def watcher(hosts, link):
    for date in (0.5, 1.5, 2.5, 3.5):
        this_actor.sleep_until(date)
        states = ", ".join(f"{h.name} {'on' if h.is_on else 'off'}" for h in hosts)
        this_actor.info(f"{states}; {link.name} {'on' if link.is_on() else 'off'}, {link.bandwidth:.0E} B/s, "
                        f"{link.latency * 1e3:.0f} ms")


if __name__ == "__main__":
    e = Engine(sys.argv)
    zone = e.netzone_root
    a = zone.add_host("A", 1e9)
    a.set_state_profile("1 0\n2 1\n", -1)
    a.set_speed_profile("0 1.0\n3 0.5\n", -1)          # speed profiles are ratios
    b = zone.add_host("B", 1e9)
    b.set_state_profile("2 0\n3 1\n", -1)
    b.set_speed_profile("0 1.0\n1 0.25\n", -1)
    a.seal()
    b.seal()
    link = zone.add_link("L", 1e8).set_latency(0.001)
    link.set_bandwidth_profile("0 1e8\n2 5e7\n", -1)  # bandwidth and latency profiles are absolute values
    link.set_latency_profile("0 0.001\n2 0.002\n", -1)
    link.set_state_profile("3 0\n4 1\n", -1)
    link.seal()
    zone.add_route(a, b, [link])
    c = zone.add_host("C", 1e9).seal()
    zone.seal()
    c.add_actor("watcher", watcher, [a, b], link)
    e.run()
