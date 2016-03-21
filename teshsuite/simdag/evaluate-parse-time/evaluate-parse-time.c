/* Copyright (c) 2008-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

//teshsuite/simdag/platforms/evaluate_parse_time ../examples/platforms/nancy.xml

#include <stdio.h>
#include "src/internal_config.h"
#if HAVE_UNISTD_H
#  include <unistd.h>
#endif

#include "simgrid/simdag.h"
#include "surf/surf.h"
#include "xbt/xbt_os_time.h"

int main(int argc, char **argv)
{
  xbt_os_timer_t timer = xbt_os_timer_new();
  SD_init(&argc, argv);

  /* creation of the environment, timed */
  xbt_os_cputimer_start(timer);
  SD_create_environment(argv[1]);
  xbt_os_cputimer_stop(timer);

  /* Display the result and exit after cleanup */
  printf( "%f\n", xbt_os_timer_elapsed(timer) );
  printf("Workstation number: %zu, link number: %d\n", sg_host_count(), sg_link_count());
  if(argv[2]){
    printf("Wait for %ss\n",argv[2]);
    sleep(atoi(argv[2]));
  }

  SD_exit();

  free(timer);
  return 0;
}
