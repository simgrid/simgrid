/* Copyright (c) 2017-2019. The SimGrid Team. All rights reserved.          */

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

#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_ptask, "Messages specific for this s4u example");

static void runner()
{
  /* Retrieve the list of all hosts as an array of hosts */
  auto hosts         = simgrid::s4u::Engine::get_instance()->get_all_hosts();
  size_t hosts_count = hosts.size();

  XBT_INFO("First, build a classical parallel task, with 1 Gflop to execute on each node, "
           "and 10MB to exchange between each pair");

  std::vector<double> computation_amounts;
  std::vector<double> communication_amounts;

  /* ------[ test 1 ]----------------- */
  computation_amounts.assign(hosts_count, 1e9 /*1Gflop*/);
  communication_amounts.assign(hosts_count * hosts_count, 0);
  for (size_t i = 0; i < hosts_count; i++)
    for (size_t j = i + 1; j < hosts_count; j++)
      communication_amounts[i * hosts_count + j] = 1e7; // 10 MB

  simgrid::s4u::this_actor::parallel_execute(hosts, computation_amounts, communication_amounts);

  /* ------[ test 2 ]----------------- */
  XBT_INFO("We can do the same with a timeout of 10 seconds enabled.");
  computation_amounts.assign(hosts_count, 1e9 /*1Gflop*/);
  communication_amounts.assign(hosts_count * hosts_count, 0);
  for (size_t i = 0; i < hosts_count; i++)
    for (size_t j = i + 1; j < hosts_count; j++)
      communication_amounts[i * hosts_count + j] = 1e7; // 10 MB

  try {
    simgrid::s4u::this_actor::exec_init(hosts, computation_amounts, communication_amounts)
        ->wait_for(10.0 /* timeout (in seconds)*/);
    xbt_die("Woops, this did not timeout as expected... Please report that bug.");
  } catch (const simgrid::TimeoutException&) {
    XBT_INFO("Caught the expected timeout exception.");
  }

  /* ------[ test 3 ]----------------- */
  XBT_INFO("Then, build a parallel task involving only computations (of different amounts) and no communication");
  computation_amounts = {3e8, 6e8, 1e9}; // 300Mflop, 600Mflop, 1Gflop
  communication_amounts.clear();         // no comm
  simgrid::s4u::this_actor::parallel_execute(hosts, computation_amounts, communication_amounts);

  /* ------[ test 4 ]----------------- */
  XBT_INFO("Then, build a parallel task with no computation nor communication (synchro only)");
  computation_amounts.clear();
  communication_amounts.clear();
  simgrid::s4u::this_actor::parallel_execute(hosts, computation_amounts, communication_amounts);

  /* ------[ test 5 ]----------------- */
  XBT_INFO("Then, Monitor the execution of a parallel task");
  computation_amounts.assign(hosts_count, 1e6 /*1Mflop*/);
  communication_amounts = {0, 1e6, 0, 0, 0, 1e6, 1e6, 0, 0};
  simgrid::s4u::ExecPtr activity =
      simgrid::s4u::this_actor::exec_init(hosts, computation_amounts, communication_amounts);
  activity->start();

  while (not activity->test()) {
    XBT_INFO("Remaining flop ratio: %.0f%%", 100 * activity->get_remaining_ratio());
    simgrid::s4u::this_actor::sleep_for(5);
  }
  activity->wait();

  XBT_INFO("Goodbye now!");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  xbt_assert(argc == 2, "Usage: %s <platform file>", argv[0]);

  e.load_platform(argv[1]);
  simgrid::s4u::Actor::create("test", simgrid::s4u::Host::by_name("MyHost1"), runner);

  e.run();
  XBT_INFO("Simulation done.");
  return 0;
}
