/* Copyright (c) 2008-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/simdag.h"
#include "src/kernel/routing/NetPoint.hpp"
#include <algorithm>
#include <cstdio>

int main(int argc, char **argv)
{
  SD_init(&argc, argv);
  SD_create_environment(argv[1]);

  xbt_dynar_t hosts = sg_hosts_as_dynar();
  std::printf("Host count: %zu, link number: %d\n", sg_host_count(), sg_link_count());

  std::vector<simgrid::kernel::routing::NetPoint*> netcardList;
  simgrid::s4u::Engine::getInstance()->getNetpointList(&netcardList);
  std::sort(netcardList.begin(), netcardList.end(),
            [](simgrid::kernel::routing::NetPoint* a, simgrid::kernel::routing::NetPoint* b) {
              return a->getName() < b->getName();
            });

  int it;
  sg_host_t host;
  xbt_dynar_foreach(hosts, it, host) {
    simgrid::kernel::routing::NetPoint* nc = host->pimpl_netpoint;
    const char *type = "buggy";
    if (nc->isRouter())
      type = "router";
    if (nc->isNetZone())
      type = "netzone";
    if (nc->isHost())
      type = "host";
    std::printf("   - Seen: \"%s\". Type: %s\n", host->getCname(), type);
  }
  xbt_dynar_free(&hosts);

  std::printf("NetCards count: %zu\n", netcardList.size());
  for (auto const& nc : netcardList)
    std::printf("   - Seen: \"%s\". Type: %s\n", nc->getCname(),
                nc->isRouter() ? "router" : (nc->isNetZone() ? "netzone" : (nc->isHost() ? "host" : "buggy")));

  return 0;
}
