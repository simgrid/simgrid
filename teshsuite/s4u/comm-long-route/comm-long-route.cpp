/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

// A communication over a route of 100,000 links. Its action uses one constraint per link, and the solver must cope
// with that many constraints in one action: neither overflow the stack nor take time growing with their square.

#include "simgrid/s4u.hpp"
#include <string>
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u test");

int main(int argc, char** argv)
{
  simgrid::s4u::Engine engine(&argc, argv);
  auto* zone  = engine.get_netzone_root()->add_netzone_full("zone");
  auto* alice = zone->add_host("alice", "1Gf");
  auto* bob   = zone->add_host("bob", "1Gf");
  std::vector<const simgrid::s4u::Link*> links;
  for (int i = 0; i < 100000; i++)
    links.push_back(zone->add_link("link" + std::to_string(i), "1GBps"));
  zone->add_route(alice, bob, links);
  zone->seal();

  alice->add_actor("sender", [alice, bob] {
    XBT_INFO("Sending");
    simgrid::s4u::Comm::sendto(alice, bob, 1e6);
    XBT_INFO("Sent");
  });

  engine.run();
  return 0;
}
