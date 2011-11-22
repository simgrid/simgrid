/* Copyright (c) 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"
#include "surf/surf_private.h"

extern routing_global_t global_routing;

int main(int argc, char **argv)
{
  /* initialisation of SD */
  int size;
  SD_init(&argc, argv);
  xbt_lib_cursor_t cursor = NULL;
  char *key, *data;

  /* creation of the environment */
  SD_create_environment(argv[1]);

  size = xbt_lib_length(host_lib) + xbt_lib_length(as_router_lib);

  printf("Workstation number: %d, link number: %d, elmts number: %d\n",
         SD_workstation_get_number(), SD_link_get_number(), size);

  xbt_lib_foreach(host_lib, cursor, key, data) {
    printf("   - Seen: \"%s\" is type : %d\n", key,
           (int) routing_get_network_element_type(key));
  }

  xbt_lib_foreach(as_router_lib, cursor, key, data) {
    printf("   - Seen: \"%s\" is type : %d\n", key,
           (int) routing_get_network_element_type(key));
  }

  SD_exit();
  return 0;
}
