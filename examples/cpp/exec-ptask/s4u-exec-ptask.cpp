/* Copyright (c) 2017-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Parallel activities are convenient abstractions of parallel computational kernels that span over several machines.
 * To create a new one, you have to provide several things:
 *   - a vector of hosts on which the activity will execute
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

  std::vector<double> computation_amounts;
  std::vector<double> communication_amounts;

  /* ------[ test 1 ]----------------- */
  XBT_INFO("First, build a classical parallel activity, with 1 Gflop to execute on each node, "
           "and 10MB to exchange between each pair");

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

  simgrid::s4u::ExecPtr activity =
      simgrid::s4u::this_actor::exec_init(hosts, computation_amounts, communication_amounts);
  try {
    activity->wait_for(10.0 /* timeout (in seconds)*/);
    xbt_die("Woops, this did not timeout as expected... Please report that bug.");
  } catch (const simgrid::TimeoutException&) {
    XBT_INFO("Caught the expected timeout exception.");
    activity->cancel();
  }

  /* ------[ test 3 ]----------------- */
  XBT_INFO("Then, build a parallel activity involving only computations (of different amounts) and no communication");
  computation_amounts = {3e8, 6e8, 1e9}; // 300Mflop, 600Mflop, 1Gflop
  communication_amounts.clear();         // no comm
  simgrid::s4u::this_actor::parallel_execute(hosts, computation_amounts, communication_amounts);

  /* ------[ test 4 ]----------------- */
  XBT_INFO("Then, build a parallel activity with no computation nor communication (synchro only)");
  computation_amounts.clear();
  communication_amounts.clear();
  simgrid::s4u::this_actor::parallel_execute(hosts, computation_amounts, communication_amounts);

  /* ------[ test 5 ]----------------- */
  XBT_INFO("Then, Monitor the execution of a parallel activity");
  computation_amounts.assign(hosts_count, 1e6 /*1Mflop*/);
  communication_amounts = {0, 1e6, 0, 0, 0, 1e6, 1e6, 0, 0};
  activity              = simgrid::s4u::this_actor::exec_init(hosts, computation_amounts, communication_amounts);
  activity->start();

  while (not activity->test()) {
    XBT_INFO("Remaining flop ratio: %.0f%%", 100 * activity->get_remaining_ratio());
    simgrid::s4u::this_actor::sleep_for(5);
  }
  activity->wait();

  /* ------[ test 6 ]----------------- */
  XBT_INFO("Finally, simulate a malleable task (a parallel execution that gets reconfigured after its start).");
  XBT_INFO("  - Start a regular parallel execution, with both comm and computation");
  computation_amounts.assign(hosts_count, 1e6 /*1Mflop*/);
  communication_amounts = {0, 1e6, 0, 0, 1e6, 0, 1e6, 0, 0};
  activity              = simgrid::s4u::this_actor::exec_init(hosts, computation_amounts, communication_amounts);
  activity->start();

  simgrid::s4u::this_actor::sleep_for(10);
  double remaining_ratio = activity->get_remaining_ratio();
  XBT_INFO("  - After 10 seconds, %.2f%% remains to be done. Change it from 3 hosts to 2 hosts only.",
           remaining_ratio * 100);
  XBT_INFO("    Let's first suspend the task.");
  activity->suspend();

  XBT_INFO("  - Now, simulate the reconfiguration (modeled as a comm from the removed host to the remaining ones).");
  std::vector<double> rescheduling_comp{0, 0, 0};
  std::vector<double> rescheduling_comm{0, 0, 0, 0, 0, 0, 25000, 25000, 0};
  simgrid::s4u::this_actor::parallel_execute(hosts, rescheduling_comp, rescheduling_comm);

  XBT_INFO("  - Now, let's cancel the old task and create a new task with modified comm and computation vectors:");
  XBT_INFO("    What was already done is removed, and the load of the removed host is shared between remaining ones.");
  for (int i = 0; i < 2; i++) {
    // remove what we've done so far, for both comm and compute load
    computation_amounts[i]   *= remaining_ratio;
    communication_amounts[i] *= remaining_ratio;
    // The work from 1 must be shared between 2 remaining ones. 1/2=50% of extra work for each
    computation_amounts[i]   *= 1.5;
    communication_amounts[i] *= 1.5;
  }
  hosts.resize(2);
  computation_amounts.resize(2);
  double remaining_comm = communication_amounts[1];
  communication_amounts = {0, remaining_comm, remaining_comm, 0}; // Resizing a linearized matrix is hairly

  activity->cancel();
  activity = simgrid::s4u::this_actor::exec_init(hosts, computation_amounts, communication_amounts);

  XBT_INFO("  - Done, let's wait for the task completion");
  activity->wait();

  XBT_INFO("Goodbye now!");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  xbt_assert(argc == 2, "Usage: %s <platform file>", argv[0]);

  e.load_platform(argv[1]);
  simgrid::s4u::Actor::create("test", e.host_by_name("MyHost1"), runner);

  e.run();
  XBT_INFO("Simulation done.");
  return 0;
}
