/* Copyright (c) 2008-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

//for i in $(seq 1 100); do teshsuite/simdag/platforms/evaluate_get_route_time ../examples/platforms/cluster_backbone.xml 1 2> /tmp/null ; done


#include <stdio.h>
#include "simgrid/simdag.h"
#include "xbt/xbt_os_time.h"

#define BILLION  1000000000L;

int main(int argc, char **argv)
{
  int i;
  int j;
  xbt_os_timer_t timer = xbt_os_timer_new();

  SD_init(&argc, argv);
  SD_create_environment(argv[1]);

  sg_host_t *hosts = sg_host_list();
  int host_count = sg_host_count();

  /* Random number initialization */
  srand( (int) (xbt_os_time()*1000) );

  do {
    i = rand()%host_count;
    j = rand()%host_count;
  } while(i==j);

  const_sg_host_t h1 = hosts[i];
  const_sg_host_t h2 = hosts[j];
  printf("%d\tand\t%d\t\t",i,j);
  xbt_dynar_t route = xbt_dynar_new(sizeof(SD_link_t), NULL);

  xbt_os_cputimer_start(timer);
  sg_host_route(h1, h2, route);
  xbt_os_cputimer_stop(timer);

  xbt_dynar_free(&route);

  printf("%f\n", xbt_os_timer_elapsed(timer) );

  xbt_free(hosts);

  return 0;
}
