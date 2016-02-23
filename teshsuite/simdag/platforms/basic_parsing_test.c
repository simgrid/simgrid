/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "simgrid/simdag.h"

int main(int argc, char **argv)
{
  SD_init(&argc, argv);

  /* creation of the environment */
  SD_create_environment(argv[1]);
  printf("Workstation number: %zu, link number: %d\n", sg_host_count(), sg_link_count());

  sg_host_t *hosts = sg_host_list();
  if (argc >= 3) {
    if (!strcmp(argv[2], "ONE_LINK")) {
      sg_host_t h1 = hosts[0];
      sg_host_t h2 = hosts[1];
      const char *name1 = sg_host_get_name(h1);
      const char *name2 = sg_host_get_name(h2);

      printf("Route between %s and %s\n", name1, name2);
      SD_link_t *route = SD_route_get_list(h1, h2);
      int route_size = SD_route_get_size(h1, h2);
      printf("Route size %d\n", route_size);
      for (int i = 0; i < route_size; i++)
        printf("  Link %s: latency = %f, bandwidth = %f\n", sg_link_name(route[i]),
               sg_link_latency(route[i]), sg_link_bandwidth(route[i]));
      printf("Route latency = %f, route bandwidth = %f\n",
             SD_route_get_latency(h1, h2), SD_route_get_bandwidth(h1, h2));
      xbt_free(route);
    }
    if (!strcmp(argv[2], "FULL_LINK")) {
      int list_size = sg_host_count();
      for (int i = 0; i < list_size; i++) {
        sg_host_t h1 = hosts[i];
        const char *name1 = sg_host_get_name(h1);
        for (int j = 0; j < list_size; j++) {
          sg_host_t h2 = hosts[j];
          const char *name2 = sg_host_get_name(h2);
          printf("Route between %s and %s\n", name1, name2);
          SD_link_t *route = SD_route_get_list(h1, h2);
          int route_size = SD_route_get_size(h1, h2);
          printf("  Route size %d\n", route_size);
          for (int k = 0; k < route_size; k++) {
            printf("  Link %s: latency = %f, bandwidth = %f\n",
                sg_link_name(route[k]), sg_link_latency(route[k]), sg_link_bandwidth(route[k]));
          }
          printf("  Route latency = %f, route bandwidth = %f\n",
                 SD_route_get_latency(h1, h2), SD_route_get_bandwidth(h1, h2));
          xbt_free(route);
        }
      }
    }
    if (!strcmp(argv[2], "PROP"))
      printf("SG_TEST_mem: %s\n", sg_host_get_property_value(sg_host_by_name("host1"), "SG_TEST_mem"));
  }
  xbt_free(hosts);

  SD_exit();
  return 0;
}
