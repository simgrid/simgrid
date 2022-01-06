/* Copyright (c) 2008-2022. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
for i in $(seq 1 20); do
  teshsuite/s4u/evaluate-get-route-time/evaluate-get-route-time examples/platforms/cluster_backbone.xml
  sleep 1
done
*/

#include "simgrid/s4u.hpp"
#include "xbt/random.hpp"
#include "xbt/xbt_os_time.h"
#include <cstdio>

int main(int argc, char** argv)
{
  xbt_os_timer_t timer = xbt_os_timer_new();

  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  e.seal_platform();

  std::vector<simgrid::s4u::Host*> hosts = e.get_all_hosts();
  int host_count                         = static_cast<int>(e.get_host_count());

  /* Random number initialization */
  simgrid::xbt::random::set_mersenne_seed(static_cast<int>(xbt_os_time()));

  /* Take random i and j, with i != j */
  xbt_assert(host_count > 1);
  int i = simgrid::xbt::random::uniform_int(0, host_count - 1);
  int j = simgrid::xbt::random::uniform_int(0, host_count - 2);
  if (j >= i) // '>=' is not a bug: j is uniform on host_count-1 values, and shifted on need to maintain uniform random
    j++;

  printf("%d\tand\t%d\t\t", i, j);

  std::vector<simgrid::s4u::Link*> route;

  xbt_os_cputimer_start(timer);
  hosts[i]->route_to(hosts[j], route, nullptr);
  xbt_os_cputimer_stop(timer);

  printf("%f\n", xbt_os_timer_elapsed(timer));

  return 0;
}
