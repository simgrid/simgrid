/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to simulate a non-linear resource sharing for
 * CPUs.
 */

#include <simgrid/s4u.hpp>

namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

/*************************************************************************************************/
static void runner()
{
  double computation_amount = sg4::this_actor::get_host()->get_speed();
  int n_task                = 10;
  std::vector<sg4::ExecPtr> tasks;

  XBT_INFO(
      "Execute %d tasks of %g flops, should take %d second in a CPU without degradation. It will take the double here.",
      n_task, computation_amount, n_task);
  for (int i = 0; i < n_task; i++) {
    tasks.emplace_back(sg4::this_actor::exec_async(computation_amount));
  }
  XBT_INFO("Waiting for all tasks to be done!");
  for (const auto& task : tasks)
    task->wait();

  XBT_INFO("Finished executing. Goodbye now!");
}
/*************************************************************************************************/
/** @brief Non-linear resource sharing for CPU */
static double cpu_nonlinear(const sg4::Host* host, double capacity, int n)
{
  /* emulates a degradation in CPU according to the number of tasks
   * totally unrealistic but for learning purposes */
  capacity = n > 1 ? capacity / 2 : capacity;
  XBT_INFO("Host %s, %d concurrent tasks, new capacity %lf", host->get_cname(), n, capacity);
  return capacity;
}

/** @brief Create a simple 1-host platform */
static void load_platform()
{
  auto* zone        = sg4::create_empty_zone("Zone1");
  auto* runner_host = zone->create_host("runner", 1e6);
  runner_host->set_sharing_policy(sg4::Host::SharingPolicy::NONLINEAR,
                                  std::bind(&cpu_nonlinear, runner_host, std::placeholders::_1, std::placeholders::_2));
  runner_host->seal();
  zone->seal();

  /* create actor runner */
  sg4::Actor::create("runner", runner_host, runner);
}

/*************************************************************************************************/
int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  /* create platform */
  load_platform();

  /* runs the simulation */
  e.run();

  return 0;
}
