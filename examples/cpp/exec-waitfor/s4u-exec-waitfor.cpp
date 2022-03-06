/* Copyright (c) 2019-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_exec_waitfor, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void worker()
{
  sg4::ExecPtr exec;
  double amount = 5 * sg4::this_actor::get_host()->get_speed();
  XBT_INFO("Create an activity that should run for 5 seconds");

  exec = sg4::this_actor::exec_async(amount);

  /* Now that execution is started, wait for 3 seconds. */
  XBT_INFO("But let it end after 3 seconds");
  try {
    exec->wait_for(3);
    XBT_INFO("Execution complete");
  } catch (const simgrid::TimeoutException&) {
    XBT_INFO("Execution Wait Timeout!");
  }

  /* do it again, but this time with a timeout greater than the duration of the execution */
  XBT_INFO("Create another activity that should run for 5 seconds and wait for it for 6 seconds");
  exec = sg4::this_actor::exec_async(amount);
  try {
    exec->wait_for(6);
    XBT_INFO("Execution complete");
  } catch (const simgrid::TimeoutException&) {
    XBT_INFO("Execution Wait Timeout!");
  }

  XBT_INFO("Finally test with a parallel execution");
  auto hosts         = sg4::Engine::get_instance()->get_all_hosts();
  size_t hosts_count = hosts.size();
  std::vector<double> computation_amounts;
  std::vector<double> communication_amounts;

  computation_amounts.assign(hosts_count, 1e9 /*1Gflop*/);
  communication_amounts.assign(hosts_count * hosts_count, 0);
  for (size_t i = 0; i < hosts_count; i++)
    for (size_t j = i + 1; j < hosts_count; j++)
      communication_amounts[i * hosts_count + j] = 1e7; // 10 MB

  exec = sg4::this_actor::exec_init(hosts, computation_amounts, communication_amounts);
  try {
    exec->wait_for(2);
    XBT_INFO("Parallel Execution complete");
  } catch (const simgrid::TimeoutException&) {
    XBT_INFO("Parallel Execution Wait Timeout!");
  }
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  sg4::Actor::create("worker", e.host_by_name("Tremblay"), worker);
  e.run();

  return 0;
}
