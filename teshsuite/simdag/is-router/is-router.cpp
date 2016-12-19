/* Copyright (c) 2008-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/engine.hpp"
#include "simgrid/s4u/host.hpp"
#include "simgrid/simdag.h"
#include "src/kernel/routing/NetCard.hpp"
#include "surf/surf_routing.h"
#include <stdio.h>

int main(int argc, char **argv)
{
  SD_init(&argc, argv);
  SD_create_environment(argv[1]);

  xbt_dynar_t hosts = sg_hosts_as_dynar();
  std::printf("Host count: %zu, link number: %d\n", sg_host_count(), sg_link_count());

  std::vector<simgrid::kernel::routing::NetCard*> netcardList;
  simgrid::s4u::Engine::instance()->netcardList(&netcardList);

  int it;
  sg_host_t host;
  xbt_dynar_foreach(hosts, it, host) {
    simgrid::kernel::routing::NetCard * nc = host->pimpl_netcard;
    std::printf("   - Seen: \"%s\". Type: %s\n", host->cname(),
                nc->isRouter() ? "router" : (nc->isNetZone() ? "netzone" : (nc->isHost() ? "host" : "buggy")));
  }
  xbt_dynar_free(&hosts);

  std::printf("NetCards count: %d\n", xbt_dict_length(netcards_dict));
  for (auto nc : netcardList)
    std::printf("   - Seen: \"%s\". Type: %s\n", nc->cname(),
                nc->isRouter() ? "router" : (nc->isNetZone() ? "netzone" : (nc->isHost() ? "host" : "buggy")));

  SD_exit();
  return 0;
}
