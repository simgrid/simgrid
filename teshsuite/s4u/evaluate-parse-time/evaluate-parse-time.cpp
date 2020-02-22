/* Copyright (c) 2008-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

// teshsuite/s4u/evaluate-parse-time/evaluate-parse-time examples/platforms/g5k.xml

#include <stdio.h>

#include "simgrid/s4u/Engine.hpp"
#include "xbt/xbt_os_time.h"

int main(int argc, char** argv)
{
  xbt_os_timer_t timer = xbt_os_timer_new();
  simgrid::s4u::Engine e(&argc, argv);

  /* creation of the environment, timed */
  xbt_os_cputimer_start(timer);
  e.load_platform(argv[1]);
  xbt_os_cputimer_stop(timer);

  /* Display the result and exit after cleanup */
  printf("%f\n", xbt_os_timer_elapsed(timer));
  printf("Host number: %zu, link number: %zu\n", e.get_host_count(), e.get_link_count());
  if (argv[2]) {
    printf("Wait for %ss\n", argv[2]);
    xbt_os_sleep(atoi(argv[2]));
  }

  xbt_os_timer_free(timer);
  return 0;
}
