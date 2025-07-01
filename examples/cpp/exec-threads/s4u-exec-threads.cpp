/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_exec_thread, "Messages specific for this s4u example");

namespace sg4 = simgrid::s4u;

static void runner()
{
  const auto* e             = sg4::Engine::get_instance();
  sg4::Host* multicore_host = e->host_by_name("MyHost1");
  // First test with less than, same number as, and more threads than cores
  double start_time = e->get_clock();
  sg4::this_actor::thread_execute(multicore_host, 1e9, 2);
  XBT_INFO("Computed 2-thread activity on a 4-core host. Took %g s", e->get_clock() - start_time);

  start_time = e->get_clock();
  sg4::this_actor::thread_execute(multicore_host, 1e9, 4);
  XBT_INFO("Computed 4-thread activity on a 4-core host. Took %g s", e->get_clock() - start_time);

  start_time = e->get_clock();
  sg4::this_actor::thread_execute(multicore_host, 1e9, 6);
  XBT_INFO("Computed 6-thread activity on a 4-core host. Took %g s", e->get_clock() - start_time);

  simgrid::s4u::ExecPtr background_task = sg4::this_actor::exec_async(2.5e9);
  XBT_INFO("Start a 1-thread background task on the 4-core host.");

  start_time = e->get_clock();
  sg4::this_actor::thread_execute(multicore_host, 1e9, 2);
  XBT_INFO("Computed 2-thread activity on a 4-core host. Took %g s", e->get_clock() - start_time);

  start_time = e->get_clock();
  sg4::this_actor::thread_execute(multicore_host, 1e9, 4);
  XBT_INFO("Computed 4-thread activity on a 4-core host. Took %g s", e->get_clock() - start_time);

  background_task->wait();
  XBT_INFO("The background task has ended.");

  background_task = sg4::this_actor::thread_execute_async(2e9, 4);
  XBT_INFO("Start a 4-thread background task on the 4-core host.");

  XBT_INFO("Sleep for 5 seconds before starting another competing task");
  sg4::this_actor::sleep_for(5);

  start_time = e->get_clock();
  sg4::this_actor::execute(1e9);
  XBT_INFO("Computed 1-thread activity on a 4-core host. Took %g s", e->get_clock() - start_time);

  background_task->wait();
  XBT_INFO("The background task has ended.");
 
  start_time = e->get_clock();
  XBT_INFO("Asynchronously start two 2-thread tasks on the 4-core host.");
  auto task1 = sg4::this_actor::thread_execute_async(1e9, 2);
  auto task2 = sg4::this_actor::thread_execute_async(1e9, 2);
  sg4::ActivitySet pending({task1, task2});
  
  while (not pending.empty()) {
    auto completed_one = pending.wait_any();
    if (completed_one != nullptr)
      XBT_INFO("Activity completed");
  }
  //pending.wait_all();
  XBT_INFO("Computed two 2-thread activities on a 4-core host. Took %g s", e->get_clock() - start_time);
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  xbt_assert(argc == 2, "Usage: %s <platform file>", argv[0]);

  e.load_platform(argv[1]);
  e.host_by_name("MyHost1")->add_actor("test", runner);

  e.run();
  XBT_INFO("Simulation done.");
  return 0;
}
