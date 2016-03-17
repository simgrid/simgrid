/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "simgrid/simdag.h"
#include "src/surf/surf_routing.hpp"

int main(int argc, char **argv)
{
  /* SD initialization */
  int size;
  xbt_lib_cursor_t cursor = NULL;
  char *key, *data;

  SD_init(&argc, argv);

  /* creation of the environment */
  SD_create_environment(argv[1]);

  size = xbt_dict_length(host_list) + xbt_lib_length(as_router_lib);

  printf("Host number: %zu, link number: %d, elmts number: %d\n", sg_host_count(), sg_link_count(), size);

  xbt_dict_foreach(host_list, cursor, key, data) {
    simgrid::surf::NetCard * nc = sg_netcard_by_name_or_null(key);
    printf("   - Seen: \"%s\". Type: %s\n", key, nc->isRouter() ? "router" : (nc->isAS()?"AS":"host"));
  }

  xbt_lib_foreach(as_router_lib, cursor, key, data) {
    simgrid::surf::NetCard * nc = sg_netcard_by_name_or_null(key);
    printf("   - Seen: \"%s\". Type: %s\n", key, nc->isRouter() ? "router" : (nc->isAS()?"AS":"host"));
  }

  SD_exit();
  return 0;
}
