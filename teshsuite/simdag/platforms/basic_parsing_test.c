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

  SD_workstation_t w1, w2;
  const SD_workstation_t *workstations;
  const SD_link_t *route;
  const char *name1;
  const char *name2;
  int route_size, i, j, k;
  int list_size;

  SD_init(&argc, argv);

  /* creation of the environment */
  SD_create_environment(argv[1]);
  printf("Workstation number: %d, link number: %d\n",
         SD_workstation_get_number(), SD_link_get_number());

  if (argc == 3) {
    if (!strcmp(argv[2], "ONE_LINK")) {
      workstations = SD_workstation_get_list();
      w1 = workstations[0];
      w2 = workstations[1];
      name1 = SD_workstation_get_name(w1);
      name2 = SD_workstation_get_name(w2);

      printf("Route between %s and %s\n", name1, name2);
      route = SD_route_get_list(w1, w2);
      route_size = SD_route_get_size(w1, w2);
      printf("Route size %d\n", route_size);
      for (i = 0; i < route_size; i++) {
        printf("   Link %s: latency = %f, bandwidth = %f\n",
               SD_link_get_name(route[i]),
               SD_link_get_current_latency(route[i]),
               SD_link_get_current_bandwidth(route[i]));
      }
      printf("Route latency = %f, route bandwidth = %f\n",
             SD_route_get_current_latency(w1, w2),
             SD_route_get_current_bandwidth(w1, w2));

    }
    if (!strcmp(argv[2], "FULL_LINK")) {
      workstations = SD_workstation_get_list();
      list_size = SD_workstation_get_number();
      for (i = 0; i < list_size; i++) {
        w1 = workstations[i];
        name1 = SD_workstation_get_name(w1);
        for (j = 0; j < list_size; j++) {
          w2 = workstations[j];
          name2 = SD_workstation_get_name(w2);
          printf("Route between %s and %s\n", name1, name2);
          route = SD_route_get_list(w1, w2);
          route_size = SD_route_get_size(w1, w2);
          printf("\tRoute size %d\n", route_size);
          for (k = 0; k < route_size; k++) {
            printf("\tLink %s: latency = %f, bandwidth = %f\n",
                   SD_link_get_name(route[k]),
                   SD_link_get_current_latency(route[k]),
                   SD_link_get_current_bandwidth(route[k]));
          }
          printf("\tRoute latency = %f, route bandwidth = %f\n",
                 SD_route_get_current_latency(w1, w2),
                 SD_route_get_current_bandwidth(w1, w2));
        }
      }

    }

  }

  SD_exit();
  return 0;
}
