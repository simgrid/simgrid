/* Copyright (c) 2008-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/routing/NetPoint.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/simdag.h"

#include <algorithm>
#include <cstdio>

int main(int argc, char **argv)
{
  SD_init(&argc, argv);
  SD_create_environment(argv[1]);

  xbt_dynar_t hosts = sg_hosts_as_dynar();
  std::printf("Host count: %zu, link number: %d\n", sg_host_count(), sg_link_count());

  std::vector<simgrid::kernel::routing::NetPoint*> netpoints =
      simgrid::s4u::Engine::get_instance()->get_all_netpoints();
  std::sort(netpoints.begin(), netpoints.end(),
            [](simgrid::kernel::routing::NetPoint* a, simgrid::kernel::routing::NetPoint* b) {
              return a->get_name() < b->get_name();
            });

  int it;
  sg_host_t host;
  xbt_dynar_foreach(hosts, it, host) {
    simgrid::kernel::routing::NetPoint* nc = host->pimpl_netpoint;
    const char *type = "buggy";
    if (nc->is_router())
      type = "router";
    if (nc->is_netzone())
      type = "netzone";
    if (nc->is_host())
      type = "host";
    std::printf("   - Seen: \"%s\". Type: %s\n", host->get_cname(), type);
  }
  xbt_dynar_free(&hosts);

  std::printf("NetCards count: %zu\n", netpoints.size());
  for (auto const& nc : netpoints)
    std::printf("   - Seen: \"%s\". Type: %s\n", nc->get_cname(),
                nc->is_router() ? "router" : (nc->is_netzone() ? "netzone" : (nc->is_host() ? "host" : "buggy")));

  return 0;
}
