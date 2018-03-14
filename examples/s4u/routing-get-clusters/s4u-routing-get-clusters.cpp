/* Copyright (c) 2009-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/ClusterZone.hpp"
#include "simgrid/kernel/routing/DragonflyZone.hpp"
#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.loadPlatform(argv[1]);

  std::vector<simgrid::kernel::routing::ClusterZone*>* clusters =
      new std::vector<simgrid::kernel::routing::ClusterZone*>;

  e.getNetzoneByType<simgrid::kernel::routing::ClusterZone>(clusters);

  for (auto c : *clusters) {
    XBT_INFO("%s", c->getCname());
    std::vector<simgrid::s4u::Host*>* hosts = new std::vector<simgrid::s4u::Host*>;
    c->getHosts(hosts);
    for (auto h : *hosts)
      XBT_INFO("   %s", h->getCname());
    delete hosts;
  }

  delete clusters;

  std::vector<simgrid::kernel::routing::DragonflyZone*>* dragonfly_clusters =
      new std::vector<simgrid::kernel::routing::DragonflyZone*>;

  e.getNetzoneByType<simgrid::kernel::routing::DragonflyZone>(dragonfly_clusters);

  if (not dragonfly_clusters->empty()) {
    for (auto d : *dragonfly_clusters) {
      XBT_INFO("%s' dragonfly topology:", d->getCname());
      for (int i = 0; i < d->getHostCount(); i++) {
        unsigned int coords[4];
        d->rankId_to_coords(i, &coords);
        XBT_INFO("   %d: (%u, %u, %u, %u)", i, coords[0], coords[1], coords[2], coords[3]);
      }
    }
  }
  delete dragonfly_clusters;

  return 0;
}
