/* Copyright (c) 2008-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"

#include <algorithm>
#include <cstdio>

int main(int argc, char** argv)
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  std::vector<sg_host_t> hosts = e.get_all_hosts();

  std::printf("Host count: %zu, link number: %zu\n", hosts.size(), e.get_link_count());
  std::vector<simgrid::kernel::routing::NetPoint*> netpoints = e.get_all_netpoints();

  std::sort(netpoints.begin(), netpoints.end(),
            [](const simgrid::kernel::routing::NetPoint* a, const simgrid::kernel::routing::NetPoint* b) {
              return a->get_name() < b->get_name();
            });

  for (const auto& host : hosts) {
    const simgrid::kernel::routing::NetPoint* nc = host->get_netpoint();
    const char* type                             = "buggy";
    if (nc->is_router())
      type = "router";
    if (nc->is_netzone())
      type = "netzone";
    if (nc->is_host())
      type = "host";
    std::printf("   - Seen: \"%s\". Type: %s\n", host->get_cname(), type);
  }

  std::printf("NetCards count: %zu\n", netpoints.size());
  for (auto const& nc : netpoints) {
    const char* type;
    if (nc->is_router())
      type = "router";
    else if (nc->is_netzone())
      type = "netzone";
    else if (nc->is_host())
      type = "host";
    else
      type = "buggy";
    std::printf("   - Seen: \"%s\". Type: %s\n", nc->get_cname(), type);
  }
  return 0;
}
