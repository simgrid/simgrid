/* Copyright (c) 2008-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "simgrid/simdag.h"
#include "src/surf/surf_routing.hpp"
#include "simgrid/s4u/host.hpp"


int main(int argc, char **argv)
{
  /* SD initialization */
  char *key, *data;

  SD_init(&argc, argv);
  SD_create_environment(argv[1]);

  xbt_dynar_t hosts = sg_hosts_as_dynar();
  int size = sg_host_count() + xbt_lib_length(as_router_lib);

  printf("Host number: %zu, link number: %d, elmts number: %d\n", sg_host_count(), sg_link_count(), size);

  int it;
  xbt_dynar_foreach(hosts, it, data) {
    sg_host_t host = (sg_host_t)data;
    simgrid::kernel::routing::NetCard * nc = host->pimpl_netcard;
    printf("   - Seen: \"%s\". Type: %s\n", host->name().c_str(), nc->isRouter() ? "router" : (nc->isAS()?"AS":"host"));
  }
  xbt_dynar_free(&hosts);

  xbt_lib_cursor_t cursor = nullptr;
  xbt_lib_foreach(as_router_lib, cursor, key, data) {
    simgrid::kernel::routing::NetCard * nc = sg_netcard_by_name_or_null(key);
    printf("   - Seen: \"%s\". Type: %s\n", key, nc->isRouter() ? "router" : (nc->isAS()?"AS":"host"));
  }

  SD_exit();
  return 0;
}
