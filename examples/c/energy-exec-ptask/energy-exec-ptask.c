/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/engine.h"
#include "simgrid/exec.h"
#include "simgrid/host.h"
#include "simgrid/plugins/energy.h"
#include "xbt/asserts.h"
#include "xbt/config.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(energy_exec_ptask, "Messages specific for this example");

static void runner(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  /* Retrieve the list of all hosts as an array of hosts */
  int host_count   = sg_host_count();
  sg_host_t* hosts = sg_host_list();

  XBT_INFO("First, build a classical parallel task, with 1 Gflop to execute on each node, "
           "and 10MB to exchange between each pair");
  double* computation_amounts   = (double*)calloc(host_count, sizeof(double));
  double* communication_amounts = (double*)calloc(host_count * host_count, sizeof(double));

  for (int i = 0; i < host_count; i++)
    computation_amounts[i] = 1e9; // 1 Gflop

  for (int i = 0; i < host_count; i++)
    for (int j = i + 1; j < host_count; j++)
      communication_amounts[i * host_count + j] = 1e7; // 10 MB

  sg_actor_parallel_execute(host_count, hosts, computation_amounts, communication_amounts);

  free(communication_amounts);
  free(computation_amounts);

  XBT_INFO("We can do the same with a timeout of one second enabled.");
  computation_amounts   = (double*)calloc(host_count, sizeof(double));
  communication_amounts = (double*)calloc(host_count * host_count, sizeof(double));
  for (int i = 0; i < host_count; i++)
    computation_amounts[i] = 1e9; // 1 Gflop
  for (int i = 0; i < host_count; i++)
    for (int j = i + 1; j < host_count; j++)
      communication_amounts[i * host_count + j] = 1e7; // 10 MB

  sg_exec_t exec = sg_actor_parallel_exec_init(host_count, hosts, computation_amounts, communication_amounts);
  sg_exec_wait_for(exec, 1 /* timeout (in seconds)*/);
  free(communication_amounts);
  free(computation_amounts);

  XBT_INFO("Then, build a parallel task involving only computations and no communication (1 Gflop per node)");
  computation_amounts = (double*)calloc(host_count, sizeof(double));
  for (int i = 0; i < host_count; i++)
    computation_amounts[i] = 1e9; // 1 Gflop
  sg_actor_parallel_execute(host_count, hosts, computation_amounts, NULL);
  free(computation_amounts);

  XBT_INFO("Then, build a parallel task with no computation nor communication (synchro only)");
  computation_amounts   = (double*)calloc(host_count, sizeof(double));
  communication_amounts = (double*)calloc(host_count * host_count, sizeof(double));
  sg_actor_parallel_execute(host_count, hosts, computation_amounts, communication_amounts);
  free(communication_amounts);
  free(computation_amounts);

  XBT_INFO("Finally, trick the ptask to do a 'remote execution', on host %s", sg_host_get_name(hosts[1]));
  computation_amounts    = (double*)calloc(1, sizeof(double));
  computation_amounts[0] = 1e9; // 1 Gflop
  sg_host_t remote[1];
  remote[0] = hosts[1];
  sg_actor_parallel_execute(1, remote, computation_amounts, NULL /* no comm */);
  free(computation_amounts);

  XBT_INFO("Goodbye now!");
  free(hosts);
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  sg_cfg_set_string("host/model", "ptask_L07");

  xbt_assert(argc <= 3, "1Usage: %s <platform file> [--energy]", argv[0]);
  xbt_assert(argc >= 2, "2Usage: %s <platform file> [--energy]", argv[0]);

  if (argc == 3 && argv[2][2] == 'e')
    sg_host_energy_plugin_init();

  simgrid_load_platform(argv[1]);

  /* Pick a process, no matter which, from the platform file */
  sg_host_t* all_hosts = sg_host_list();
  sg_host_t first_host = all_hosts[0];
  free(all_hosts);

  sg_actor_t actor = sg_actor_init("test", first_host);
  sg_actor_start(actor, runner, 0, NULL);

  simgrid_run();
  XBT_INFO("Simulation done.");

  return 0;
}
