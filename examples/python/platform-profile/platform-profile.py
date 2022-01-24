# Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the license (GNU LGPL) which comes with this package.

from simgrid import Actor, Comm, Engine, Host, Mailbox, Link, this_actor
import sys

# This example demonstrates how to attach a profile to a host or a link, to specify external changes to the resource speed.
# The first way to do so is to use a file in the XML, while the second is to use the programmatic interface.

def watcher():
  jupiter  = Host.by_name("Jupiter")
  fafard   = Host.by_name("Fafard")
  lilibeth = Host.by_name("Lilibeth")
  link1    = Link.by_name("1")
  link2    = Link.by_name("2")

  (links, lat) = jupiter.route_to(fafard)
  path = ""
  for l in links:
    path += ("" if len(path)==0 else ", ") + "link '" + l.name + "'"
  this_actor.info(f"Path from Jupiter to Fafard: {path} (latency: {lat:.6f}s).")

  for _ in range(10):
    this_actor.info("Fafard: %.0fMflops, Jupiter: %4.0fMflops, Lilibeth: %3.1fMflops, Link1: (%.2fMB/s %.0fms), Link2: (%.2fMB/s %.0fms)" % (
             fafard.speed * fafard.available_speed / 1000000,
             jupiter.speed * jupiter.available_speed / 1000000,
             lilibeth.speed * lilibeth.available_speed / 1000000, 
             link1.bandwidth / 1000, link1.latency * 1000, 
             link2.bandwidth / 1000, link2.latency * 1000))
    this_actor.sleep_for(1)

if __name__ == '__main__':
  e = Engine(sys.argv)
  # Load the platform description
  e.load_platform(sys.argv[1])

  # Add a new host programmatically, and attach a simple speed profile to it (alternate between full and half speed every two seconds
  lili = e.get_netzone_root().create_host("Lilibeth", 25e6)
  lili.set_speed_profile("""0 1.0
2 0.5""", 2)
  lili.seal()

  # Add a watcher of the changes
  Actor.create("watcher", Host.by_name("Fafard"), watcher)

  e.run()
