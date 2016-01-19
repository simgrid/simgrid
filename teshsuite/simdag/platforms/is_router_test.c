/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "src/surf/surf_private.h"

extern routing_platf_t routing_platf;

int main(int argc, char **argv)
{
  /* SD initialization */
  int size;
  xbt_lib_cursor_t cursor = NULL;
  char *key, *data;

#ifdef _XBT_WIN32
  setbuf(stderr, NULL);
  setbuf(stdout, NULL);
#endif

  SD_init(&argc, argv);

  /* creation of the environment */
  SD_create_environment(argv[1]);

  size = xbt_dict_length(host_list) + xbt_lib_length(as_router_lib);

  printf("Workstation number: %d, link number: %d, elmts number: %d\n",
         SD_workstation_get_count(), sg_link_count(), size);

  xbt_dict_foreach(host_list, cursor, key, data) {
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
