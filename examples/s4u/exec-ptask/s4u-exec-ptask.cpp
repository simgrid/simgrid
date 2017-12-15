/* Copyright (c) 2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Parallel tasks are convenient abstractions of parallel computational kernels that span over several machines.
 * To create a new one, you have to provide several things:
 *   - a vector of hosts on which the task will execute
 *   - a vector of values, the amount of computation for each of the hosts (in flops)
 *   - a matrix of values, the amount of communication between each pair of hosts (in bytes)
 *
 * Each of these operation will be processed at the same relative speed.
 * This means that at some point in time, all sub-executions and all sub-communications will be at 20% of completion.
 * Also, they will all complete at the exact same time.
 *
 * This is obviously a simplistic abstraction, but this is very handful in a large amount of situations.
 *
 * Please note that you must have the LV07 platform model enabled to use such constructs.
 */

#include "simgrid/plugins/energy.h"
#include <simgrid/s4u.hpp>
#include <xbt/ex.hpp>
#include <xbt/log.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_energyptask, "Messages specific for this s4u example");

static void runner()
{
  /* Retrieve the list of all hosts as an array of hosts */
  int hosts_count            = sg_host_count();
  simgrid::s4u::Host** hosts = sg_host_list();

  XBT_INFO("First, build a classical parallel task, with 1 Gflop to execute on each node, "
           "and 10MB to exchange between each pair");
  double* computation_amounts   = new double[hosts_count]();
  double* communication_amounts = new double[hosts_count * hosts_count]();

  for (int i               = 0; i < hosts_count; i++)
    computation_amounts[i] = 1e9; // 1 Gflop

  for (int i = 0; i < hosts_count; i++)
    for (int j = i + 1; j < hosts_count; j++)
      communication_amounts[i * hosts_count + j] = 1e7; // 10 MB

  simgrid::s4u::this_actor::parallel_execute(hosts_count, hosts, computation_amounts, communication_amounts);

  XBT_INFO("We can do the same with a timeout of one second enabled.");
  computation_amounts   = new double[hosts_count]();
  communication_amounts = new double[hosts_count * hosts_count]();

  for (int i               = 0; i < hosts_count; i++)
    computation_amounts[i] = 1e9; // 1 Gflop

  for (int i = 0; i < hosts_count; i++)
    for (int j = i + 1; j < hosts_count; j++)
      communication_amounts[i * hosts_count + j] = 1e7; // 10 MB

  try {
    simgrid::s4u::this_actor::parallel_execute(hosts_count, hosts, computation_amounts, communication_amounts,
                                               1.0 /* timeout (in seconds)*/);
    XBT_WARN("Woops, this did not timeout as expected... Please report that bug.");
  } catch (xbt_ex& e) {
    /* Do nothing this exception on timeout was expected */
    XBT_DEBUG("Caught expected exception: %s", e.what());
  }

  XBT_INFO("Then, build a parallel task involving only computations and no communication (1 Gflop per node)");
  computation_amounts = new double[hosts_count]();
  for (int i               = 0; i < hosts_count; i++)
    computation_amounts[i] = 1e9; // 1 Gflop
  simgrid::s4u::this_actor::parallel_execute(hosts_count, hosts, computation_amounts, nullptr /* no comm */);

  XBT_INFO("Then, build a parallel task with no computation nor communication (synchro only)");
  computation_amounts   = new double[hosts_count]();
  communication_amounts = new double[hosts_count * hosts_count]();
  simgrid::s4u::this_actor::parallel_execute(hosts_count, hosts, computation_amounts, communication_amounts);

  XBT_INFO("Finally, trick the ptask to do a 'remote execution', on host %s", hosts[1]->getCname());
  computation_amounts = new double[1]{1e9};

  simgrid::s4u::Host* remote[] = {hosts[1]};
  simgrid::s4u::this_actor::parallel_execute(1, remote, computation_amounts, nullptr);

  XBT_INFO("Goodbye now!");
  std::free(hosts);
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  xbt_assert(argc <= 3, "1Usage: %s <platform file> [--energy]", argv[0]);
  xbt_assert(argc >= 2, "2Usage: %s <platform file> [--energy]", argv[0]);

  if (argc == 3 && argv[2][2] == 'e')
    sg_host_energy_plugin_init();

  e.loadPlatform(argv[1]);

  /* Pick a process, no matter which, from the platform file */
  simgrid::s4u::Actor::createActor("test", simgrid::s4u::Host::by_name("MyHost1"), runner);

  e.run();
  XBT_INFO("Simulation done.");
  return 0;
}
