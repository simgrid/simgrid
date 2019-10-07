/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/energy.h"
#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

/** @addtogroup MSG_examples
 *
 * - <b>energy-ptask/energy-ptask.c</b>: Demonstrates the use of @ref MSG_parallel_task_create, to create special
 *   tasks that run on several hosts at the same time. The resulting simulations are very close to what can be
 *   achieved in @ref SD_API, but still allows one to use the other features of MSG (it'd be cool to be able to mix
 *   interfaces, but it's not possible ATM).
 */

static int runner(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  /* Retrieve the list of all hosts as an array of hosts */
  int host_count    = MSG_get_host_number();
  msg_host_t* hosts = xbt_dynar_to_array(MSG_hosts_as_dynar());

  XBT_INFO("First, build a classical parallel task, with 1 Gflop to execute on each node, "
           "and 10MB to exchange between each pair");
  double* computation_amounts   = xbt_new0(double, host_count);
  double* communication_amounts = xbt_new0(double, host_count* host_count);

  for (int i = 0; i < host_count; i++)
    computation_amounts[i] = 1e9; // 1 Gflop

  for (int i = 0; i < host_count; i++)
    for (int j = i + 1; j < host_count; j++)
      communication_amounts[i * host_count + j] = 1e7; // 10 MB

  msg_task_t ptask =
      MSG_parallel_task_create("parallel task", host_count, hosts, computation_amounts, communication_amounts, NULL);
  MSG_parallel_task_execute(ptask);
  MSG_task_destroy(ptask);
  xbt_free(communication_amounts);
  xbt_free(computation_amounts);

  XBT_INFO("We can do the same with a timeout of one second enabled.");
  computation_amounts   = xbt_new0(double, host_count);
  communication_amounts = xbt_new0(double, host_count* host_count);
  for (int i = 0; i < host_count; i++)
    computation_amounts[i] = 1e9; // 1 Gflop
  for (int i = 0; i < host_count; i++)
    for (int j = i + 1; j < host_count; j++)
      communication_amounts[i * host_count + j] = 1e7; // 10 MB
  ptask =
      MSG_parallel_task_create("parallel task", host_count, hosts, computation_amounts, communication_amounts, NULL);
  msg_error_t errcode = MSG_parallel_task_execute_with_timeout(ptask, 1 /* timeout (in seconds)*/);
  xbt_assert(errcode == MSG_TIMEOUT, "Woops, this did not timeout as expected... Please report that bug.");
  MSG_task_destroy(ptask);
  xbt_free(communication_amounts);
  xbt_free(computation_amounts);

  XBT_INFO("Then, build a parallel task involving only computations and no communication (1 Gflop per node)");
  computation_amounts = xbt_new0(double, host_count);
  for (int i = 0; i < host_count; i++)
    computation_amounts[i] = 1e9; // 1 Gflop
  ptask = MSG_parallel_task_create("parallel exec", host_count, hosts, computation_amounts, NULL /* no comm */, NULL);
  MSG_parallel_task_execute(ptask);
  MSG_task_destroy(ptask);
  xbt_free(computation_amounts);

  XBT_INFO("Then, build a parallel task with no computation nor communication (synchro only)");
  computation_amounts   = xbt_new0(double, host_count);
  communication_amounts = xbt_new0(double, host_count* host_count); /* memset to 0 by xbt_new0 */
  ptask =
      MSG_parallel_task_create("parallel sync", host_count, hosts, computation_amounts, communication_amounts, NULL);
  MSG_parallel_task_execute(ptask);
  MSG_task_destroy(ptask);
  xbt_free(communication_amounts);
  xbt_free(computation_amounts);

  XBT_INFO("Finally, trick the ptask to do a 'remote execution', on host %s", MSG_host_get_name(hosts[1]));
  computation_amounts    = xbt_new0(double, 1);
  computation_amounts[0] = 1e9; // 1 Gflop
  msg_host_t* remote     = xbt_new(msg_host_t, 1);
  remote[0]              = hosts[1];
  ptask = MSG_parallel_task_create("remote exec", 1, remote, computation_amounts, NULL /* no comm */, NULL);
  MSG_parallel_task_execute(ptask);
  MSG_task_destroy(ptask);
  xbt_free(remote);
  xbt_free(computation_amounts);

  XBT_INFO("Goodbye now!");
  xbt_free(hosts);
  return 0;
}

int main(int argc, char* argv[])
{
  MSG_init(&argc, argv);
  MSG_config("host/model", "ptask_L07");

  xbt_assert(argc <= 3, "1Usage: %s <platform file> [--energy]", argv[0]);
  xbt_assert(argc >= 2, "2Usage: %s <platform file> [--energy]", argv[0]);

  if (argc == 3 && argv[2][2] == 'e')
    sg_host_energy_plugin_init();

  MSG_create_environment(argv[1]);

  /* Pick a process, no matter which, from the platform file */
  xbt_dynar_t all_hosts = MSG_hosts_as_dynar();
  msg_host_t first_host = xbt_dynar_getfirst_as(all_hosts, msg_host_t);
  xbt_dynar_free(&all_hosts);

  MSG_process_create("test", runner, NULL, first_host);
  msg_error_t res = MSG_main();
  XBT_INFO("Simulation done.");

  return res != MSG_OK;
}
