/* Copyright (c) 2009-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/ClusterZone.hpp"
#include "simgrid/kernel/routing/DragonflyZone.hpp"
#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  std::vector<simgrid::kernel::routing::ClusterZone*> clusters =
      e.get_filtered_netzones<simgrid::kernel::routing::ClusterZone>();

  for (auto c : clusters) {
    XBT_INFO("%s", c->get_cname());
    std::vector<simgrid::s4u::Host*> hosts = c->get_all_hosts();
    for (auto h : hosts)
      XBT_INFO("   %s", h->get_cname());
  }

  std::vector<simgrid::kernel::routing::DragonflyZone*> dragonfly_clusters =
      e.get_filtered_netzones<simgrid::kernel::routing::DragonflyZone>();

  for (auto d : dragonfly_clusters) {
    XBT_INFO("%s' dragonfly topology:", d->get_cname());
    for (int i = 0; i < d->get_host_count(); i++) {
      const simgrid::kernel::routing::DragonflyZone::Coords coords = d->rankId_to_coords(i);
      XBT_INFO("   %d: (%lu, %lu, %lu, %lu)", i, coords.group, coords.chassis, coords.blade, coords.node);
    }
  }

  return 0;
}
