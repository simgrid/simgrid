/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "simgrid/simdag.h"

int main(int argc, char **argv)
{
  /* SD initialization */

  sg_host_t w1, w2;
  sg_host_t *workstations;
  SD_link_t *route;
  const char *name1;
  const char *name2;
  int route_size, i, j, k;
  int list_size;

  SD_init(&argc, argv);

  /* creation of the environment */
  SD_create_environment(argv[1]);
  printf("Workstation number: %zu, link number: %d\n",
         sg_host_count(), sg_link_count());

  workstations = sg_host_list();
  if (argc >= 3) {
    if (!strcmp(argv[2], "ONE_LINK")) {
      w1 = workstations[0];
      w2 = workstations[1];
      name1 = sg_host_get_name(w1);
      name2 = sg_host_get_name(w2);

      printf("Route between %s and %s\n", name1, name2);
      route = SD_route_get_list(w1, w2);
      route_size = SD_route_get_size(w1, w2);
      printf("Route size %d\n", route_size);
      for (i = 0; i < route_size; i++) {
      printf("  Link %s: latency = %f, bandwidth = %f\n",
           sg_link_name(route[i]),
           sg_link_latency(route[i]),
           sg_link_bandwidth(route[i]));
      }
      printf("Route latency = %f, route bandwidth = %f\n",
         SD_route_get_latency(w1, w2),
         SD_route_get_bandwidth(w1, w2));
      xbt_free(route);
    }
    if (!strcmp(argv[2], "FULL_LINK")) {
      list_size = sg_host_count();
      for (i = 0; i < list_size; i++) {
      w1 = workstations[i];
      name1 = sg_host_get_name(w1);
      for (j = 0; j < list_size; j++) {
        w2 = workstations[j];
        name2 = sg_host_get_name(w2);
        printf("Route between %s and %s\n", name1, name2);
        route = SD_route_get_list(w1, w2);
        route_size = SD_route_get_size(w1, w2);
        printf("  Route size %d\n", route_size);
        for (k = 0; k < route_size; k++) {
        printf("  Link %s: latency = %f, bandwidth = %f\n",
             sg_link_name(route[k]),
             sg_link_latency(route[k]),
             sg_link_bandwidth(route[k]));
        }
        printf("  Route latency = %f, route bandwidth = %f\n",
           SD_route_get_latency(w1, w2),
           SD_route_get_bandwidth(w1, w2));
        xbt_free(route);
      }
      }
    }
    if (!strcmp(argv[2], "PROP")) {
      printf("SG_TEST_mem: %s\n",
          sg_host_get_property_value(sg_host_by_name("host1"),
          "SG_TEST_mem"));
    }
  }
  xbt_free(workstations);

  SD_exit();
  return 0;
}
