/* Copyright (c) 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "simdag/simdag.h"

int main(int argc, char **argv)
{
  /* initialisation of SD */

  SD_workstation_t w1, w2;
  const SD_workstation_t *workstations;
  const SD_link_t *route;
  const char *name1;
  const char *name2;
  int route_size,i;

  SD_init(&argc, argv);

  /* creation of the environment */
  SD_create_environment(argv[1]);

  printf("\n\nWorkstation number: %d, link number: %d\n\n",
         SD_workstation_get_number(), SD_link_get_number());

  workstations = SD_workstation_get_list();
  w1 = workstations[0];
  w2 = workstations[1];
  name1 = SD_workstation_get_name(w1);
  name2 = SD_workstation_get_name(w2);

  printf("Route between %s and %s\n", name1, name2);
  route = SD_route_get_list(w1, w2);
  route_size = SD_route_get_size(w1, w2);
  for (i = 0; i < route_size; i++) {
	  printf("   Link %s: latency = %f, bandwidth = %f\n",
          SD_link_get_name(route[i]), SD_link_get_current_latency(route[i]),
          SD_link_get_current_bandwidth(route[i]));
  }
  printf("Route latency = %f, route bandwidth = %fn",
        SD_route_get_current_latency(w1, w2),
        SD_route_get_current_bandwidth(w1, w2));

  SD_exit();
  return 0;
}
