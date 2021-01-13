/* Latency tests                                                            */

/* Copyright (c) 2007-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/simdag.h"
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(comm_mxn_independent, sd, "SimDag test independent communications");

/*
 * intra communication test
 * independent communication
 *
 * 0 -> 1
 * 2 -> 3
 * shared is only switch which is fat pipe
 * should be 1 + 2 latency = 3
 */

int main(int argc, char **argv)
{
  double communication_amount[] = { 0.0, 1.0, 0.0, 0.0,
                                    0.0, 0.0, 0.0, 0.0,
                                    0.0, 0.0, 0.0, 1.0,
                                    0.0, 0.0, 0.0, 0.0 };

  SD_init(&argc, argv);
  SD_create_environment(argv[1]);

  SD_task_t task = SD_task_create("Comm 1", NULL, 1.0);

  sg_host_t *hosts = sg_host_list();
  SD_task_schedule(task, 4, hosts, SD_SCHED_NO_COST, communication_amount, -1.0);
  xbt_free(hosts);

  SD_simulate(-1.0);

  XBT_INFO("%g", SD_get_clock());

  SD_task_destroy(task);

  return 0;
}
