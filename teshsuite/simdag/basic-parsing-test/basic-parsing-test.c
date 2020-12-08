/* Copyright (c) 2008-2020. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "simgrid/simdag.h"

static void test_one_link(const sg_host_t* hosts)
{
  const_sg_host_t h1 = hosts[0];
  const_sg_host_t h2 = hosts[1];
  const char* name1  = sg_host_get_name(h1);
  const char* name2  = sg_host_get_name(h2);

  fprintf(stderr, "Route between %s and %s\n", name1, name2);
  xbt_dynar_t route = xbt_dynar_new(sizeof(SD_link_t), NULL);
  sg_host_get_route(h1, h2, route);
  fprintf(stderr, "Route size %lu\n", xbt_dynar_length(route));
  unsigned int i;
  SD_link_t link;
  xbt_dynar_foreach (route, i, link)
    fprintf(stderr, "  Link %s: latency = %f, bandwidth = %f\n", sg_link_get_name(link), sg_link_get_latency(link),
            sg_link_get_bandwidth(link));
  fprintf(stderr, "Route latency = %f, route bandwidth = %f\n", sg_host_get_route_latency(h1, h2),
          sg_host_get_route_bandwidth(h1, h2));
  xbt_dynar_free_container(&route);
}

static void test_full_link(const sg_host_t* hosts)
{
  size_t list_size = sg_host_count();
  for (size_t i = 0; i < list_size; i++) {
    const_sg_host_t h1 = hosts[i];
    const char* name1  = sg_host_get_name(h1);
    for (size_t j = 0; j < list_size; j++) {
      const_sg_host_t h2 = hosts[j];
      const char* name2  = sg_host_get_name(h2);
      fprintf(stderr, "Route between %s and %s\n", name1, name2);
      xbt_dynar_t route = xbt_dynar_new(sizeof(SD_link_t), NULL);
      sg_host_get_route(h1, h2, route);
      fprintf(stderr, "  Route size %lu\n", xbt_dynar_length(route));
      unsigned int k;
      SD_link_t link;
      xbt_dynar_foreach (route, k, link)
        fprintf(stderr, "  Link %s: latency = %f, bandwidth = %f\n", sg_link_get_name(link), sg_link_get_latency(link),
                sg_link_get_bandwidth(link));
      fprintf(stderr, "  Route latency = %f, route bandwidth = %f\n", sg_host_get_route_latency(h1, h2),
              sg_host_get_route_bandwidth(h1, h2));
      xbt_dynar_free_container(&route);
    }
  }
}

int main(int argc, char** argv)
{
  SD_init(&argc, argv);

  /* creation of the environment */
  SD_create_environment(argv[1]);
  fprintf(stderr, "Workstation number: %zu, link number: %d\n", sg_host_count(), sg_link_count());

  sg_host_t* hosts = sg_host_list();
  if (argc >= 3) {
    if (!strcmp(argv[2], "ONE_LINK"))
      test_one_link(hosts);
    if (!strcmp(argv[2], "FULL_LINK"))
      test_full_link(hosts);
    if (!strcmp(argv[2], "PROP"))
      fprintf(stderr,"SG_TEST_mem: %s\n", sg_host_get_property_value(sg_host_by_name("host1"), "SG_TEST_mem"));
  }
  xbt_free(hosts);

  return 0;
}
