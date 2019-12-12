/* Copyright (c) 2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_exec_waitfor, "Messages specific for this s4u example");

static void worker()
{
  simgrid::s4u::ExecPtr exec;
  double amount = 5 * simgrid::s4u::this_actor::get_host()->get_speed();
  XBT_INFO("Create an activity that should run for 5 seconds");

  exec = simgrid::s4u::this_actor::exec_async(amount);

  /* Now that execution is started, wait for 3 seconds. */
  XBT_INFO("But let it end after 3 seconds");
  try {
    exec->wait_for(3);
    XBT_INFO("Execution complete");
  } catch (simgrid::TimeoutException&) {
    XBT_INFO("Execution Timeout!");
  }

  /* do it again, but this time with a timeout greater than the duration of the execution */
  XBT_INFO("Create another activity that should run for 5 seconds and wait for it for 6 seconds");
  exec = simgrid::s4u::this_actor::exec_async(amount);
  try {
    exec->wait_for(6);
    XBT_INFO("Execution complete");
  } catch (simgrid::TimeoutException&) {
    XBT_INFO("Execution Timeout!");
  }

  XBT_INFO("Finally test with a parallel execution");
  auto hosts         = simgrid::s4u::Engine::get_instance()->get_all_hosts();
  size_t hosts_count = hosts.size();
  std::vector<double> computation_amounts;
  std::vector<double> communication_amounts;

  computation_amounts.assign(hosts_count, 1e9 /*1Gflop*/);
  communication_amounts.assign(hosts_count * hosts_count, 0);
  for (size_t i = 0; i < hosts_count; i++)
    for (size_t j = i + 1; j < hosts_count; j++)
      communication_amounts[i * hosts_count + j] = 1e7; // 10 MB

  exec = simgrid::s4u::this_actor::exec_init(hosts, computation_amounts, communication_amounts);
  try {
    exec->wait_for(2);
    XBT_INFO("Parallel Execution complete");
  } catch (simgrid::TimeoutException&) {
    XBT_INFO("Parallel Execution Timeout!");
  }
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  simgrid::s4u::Actor::create("worker", simgrid::s4u::Host::by_name("Tremblay"), worker);
  e.run();

  return 0;
}
