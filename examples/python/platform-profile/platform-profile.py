# Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

"""
This example demonstrates how to attach a profile to a host or a link, to specify external changes to the resource
speed.
The first way to do so is to use a file in the XML, while the second is to use the programmatic interface.
"""

import sys
from simgrid import Actor, Engine, Host, Link, this_actor

def watcher():
    jupiter = Host.by_name("Jupiter")
    fafard = Host.by_name("Fafard")
    lilibeth = Host.by_name("Lilibeth")
    link1 = Link.by_name("1")
    link2 = Link.by_name("2")

    (links, lat) = jupiter.route_to(fafard)
    path = ""
    for l in links:
        path += ("" if not path else ", ") + "link '" + l.name + "'"
    this_actor.info(f"Path from Jupiter to Fafard: {path} (latency: {lat:.6f}s).")

    for _ in range(10):
        this_actor.info("Fafard: %.0fMflops, Jupiter: %4.0fMflops, Lilibeth: %3.1fMflops, \
Link1: (%.2fMB/s %.0fms), Link2: (%.2fMB/s %.0fms)" % (fafard.speed * fafard.available_speed / 1000000,
                                                       jupiter.speed * jupiter.available_speed / 1000000,
                                                       lilibeth.speed * lilibeth.available_speed / 1000000,
                                                       link1.bandwidth / 1000, link1.latency * 1000,
                                                       link2.bandwidth / 1000, link2.latency * 1000))
        this_actor.sleep_for(1)

if __name__ == '__main__':
    e = Engine(sys.argv)
    # Load the platform description
    e.load_platform(sys.argv[1])

    # Add a new host programmatically, and attach a simple speed profile to it (alternate between full and half speed
    # every two seconds
    lili = e.netzone_root.add_host("Lilibeth", 25e6)
    lili.set_speed_profile("""0 1.0
    2 0.5""", 4)
    lili.seal()

    # Add a watcher of the changes
    e.host_by_name("Fafard").add_actor("watcher", watcher)

    e.run()
