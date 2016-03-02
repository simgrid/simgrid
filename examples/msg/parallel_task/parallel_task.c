/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

/** @addtogroup MSG_examples
 * 
 * - <b>parallel_task/parallel_task.c</b>: Demonstrates the use of @ref MSG_parallel_task_create, to create special
 *   tasks that run on several hosts at the same time. The resulting simulations are very close to what can be
 *   achieved in @ref SD_API, but still allows to use the other features of MSG (it'd be cool to be able to mix
 *   interfaces, but it's not possible ATM).
 */

static int runner(int argc, char *argv[])
{
  /* Retrieve the list of all hosts as an array of hosts */
  xbt_dynar_t slaves_dynar = MSG_hosts_as_dynar();
  int slaves_count = xbt_dynar_length(slaves_dynar);
  msg_host_t *slaves = xbt_dynar_to_array(slaves_dynar);

  XBT_INFO("First, build a classical parallel task, with 1 Gflop to execute on each node, "
           "and 10MB to exchange between each pair");
  double *computation_amounts = xbt_new0(double, slaves_count);
  double *communication_amounts = xbt_new0(double, slaves_count * slaves_count);

  for (int i = 0; i < slaves_count; i++)
    computation_amounts[i] = 1e9; // 1 Gflop

  for (int i = 0; i < slaves_count; i++)
    for (int j = i + 1; j < slaves_count; j++)
      communication_amounts[i * slaves_count + j] = 1e7; // 10 MB

  msg_task_t ptask =
    MSG_parallel_task_create("parallel task", slaves_count, slaves, computation_amounts, communication_amounts, NULL);
  MSG_parallel_task_execute(ptask);
  MSG_task_destroy(ptask);
  /* The arrays communication_amounts and computation_amounts are not to be freed manually */

  XBT_INFO("Then, build a parallel task involving only computations and no communication (1 Gflop per node)");
  computation_amounts = xbt_new0(double, slaves_count);
  for (int i = 0; i < slaves_count; i++)
    computation_amounts[i] = 1e9; // 1 Gflop
  ptask =
    MSG_parallel_task_create("parallel exec", slaves_count, slaves, computation_amounts, NULL/* no comm */, NULL);
  MSG_parallel_task_execute(ptask);
  MSG_task_destroy(ptask);

  XBT_INFO("Finally, trick the ptask to do a 'remote execution', on host %s", MSG_host_get_name(slaves[1]));
  computation_amounts = xbt_new0(double, 1);
  computation_amounts[0] = 1e9; // 1 Gflop
  msg_host_t *remote = xbt_new(msg_host_t,1);
  remote[0] = slaves[1];
  ptask = MSG_parallel_task_create("remote exec", 1, remote, computation_amounts, NULL/* no comm */, NULL);
  MSG_parallel_task_execute(ptask);
  MSG_task_destroy(ptask);
  free(remote);

  XBT_INFO("Goodbye now!");
  free(slaves);
  return 0;
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  MSG_config("host/model", "ptask_L07");

  xbt_assert(argc > 1, "Usage: %s <platform file>", argv[0]);
  MSG_create_environment(argv[1]);

  /* Pick a process, no matter which, from the platform file */
  xbt_dynar_t all_hosts = MSG_hosts_as_dynar();
  msg_host_t first_host = xbt_dynar_getfirst_as(all_hosts,msg_host_t);
  xbt_dynar_free(&all_hosts);

  MSG_process_create("test", runner, NULL, first_host);
  msg_error_t res = MSG_main();
  XBT_INFO("Simulation done.");

  return res != MSG_OK;
}
