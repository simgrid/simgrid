/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_ptask_multicore, "Messages specific for this s4u example");

namespace sg4 = simgrid::s4u;

static void runner()
{
  auto e = sg4::Engine::get_instance();
  std::vector<double> comp(2, 1e9);
  std::vector<double> comm(4, 0.0);
  // Different hosts.
  std::vector<sg4::Host*> hosts_diff = {e->host_by_name("MyHost2"), e->host_by_name("MyHost3")};
  double start_time                  = sg4::Engine::get_clock();
  sg4::this_actor::parallel_execute(hosts_diff, comp, comm);
  XBT_INFO("Computed 2-core activity on two different hosts. Took %g s", e->get_clock() - start_time);

  // Same host, monocore.
  std::vector<sg4::Host*> monocore_hosts = {e->host_by_name("MyHost2"), e->host_by_name("MyHost2")};
  start_time                             = sg4::Engine::get_clock();
  sg4::this_actor::parallel_execute(monocore_hosts, comp, comm);
  XBT_INFO("Computed 2-core activity a 1-core host. Took %g s", e->get_clock() - start_time);

  // Same host, multicore.
  std::vector<sg4::Host*> multicore_host = {e->host_by_name("MyHost1"), e->host_by_name("MyHost1")};
  start_time                             = sg4::Engine::get_clock();
  sg4::this_actor::parallel_execute(multicore_host, comp, comm);
  XBT_INFO("Computed 2-core activity on a 4-core host. Took %g s", e->get_clock() - start_time);

  // Same host, using too many cores
  std::vector<double> comp6(6, 1e9);
  std::vector<double> comm6(36, 0.0);
  std::vector<sg4::Host*> multicore_overload(6, e->host_by_name("MyHost1"));
  start_time = sg4::Engine::get_clock();
  sg4::this_actor::parallel_execute(multicore_overload, comp6, comm6);
  XBT_INFO("Computed 6-core activity on a 4-core host. Took %g s", e->get_clock() - start_time);

  // Same host, adding some communication
  std::vector<double> comm2 = {0, 1E7, 1E7, 0};
  start_time                = sg4::Engine::get_clock();
  sg4::this_actor::parallel_execute(multicore_host, comp, comm2);
  XBT_INFO("Computed 2-core activity on a 4-core host with some communication. Took %g s", e->get_clock() - start_time);

  // See if the multicore execution continues to work after changing pstate
  XBT_INFO("Switching machine multicore to pstate 1.");
  e->host_by_name("MyHost1")->set_pstate(1);
  XBT_INFO("Switching back to pstate 0.");
  e->host_by_name("MyHost1")->set_pstate(0);

  start_time = sg4::Engine::get_clock();
  sg4::this_actor::parallel_execute(multicore_host, comp, comm);
  XBT_INFO("Computed 2-core activity on a 4-core host. Took %g s", e->get_clock() - start_time);

  start_time = sg4::Engine::get_clock();
  sg4::this_actor::parallel_execute(hosts_diff, comp, comm);
  XBT_INFO("Computed 2-core activity on two different hosts. Took %g s", e->get_clock() - start_time);

  // Add a background task and change ptask on the fly
  auto MyHost1                          = e->host_by_name("MyHost1");
  simgrid::s4u::ExecPtr background_task = MyHost1->exec_async(5e9);
  XBT_INFO("Start a 1-core background task on the 4-core host.");

  start_time = sg4::Engine::get_clock();
  sg4::this_actor::parallel_execute(multicore_host, comp, comm);
  XBT_INFO("Computed 2-core activity on the 4-core host. Took %g s", e->get_clock() - start_time);
  XBT_INFO("Remaining amount of work for the background task: %.0f%%",
           100 * background_task->get_remaining_ratio());

  XBT_INFO("Switching to pstate 1 while background task is still running.");
  MyHost1->set_pstate(1);
  start_time = sg4::Engine::get_clock();
  sg4::this_actor::parallel_execute(multicore_host, comp, comm);
  XBT_INFO("Computed again the same 2-core activity on it. Took %g s", e->get_clock() - start_time);

  background_task->wait();
  XBT_INFO("The background task has ended.");
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  xbt_assert(argc == 2, "Usage: %s <platform file>", argv[0]);

  e.load_platform(argv[1]);
  sg4::Actor::create("test", e.host_by_name("MyHost1"), runner);

  e.run();
  XBT_INFO("Simulation done.");
  return 0;
}
