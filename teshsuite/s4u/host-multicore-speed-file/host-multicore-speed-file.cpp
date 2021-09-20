/* Copyright (c) 2010-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/kernel/ProfileBuilder.hpp"
#include "simgrid/s4u.hpp"

namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void run_activities(int n, double size, double rate = -1.0)
{
  double start = sg4::Engine::get_clock();
  std::vector<sg4::ExecPtr> activities;

  // initialize exec activities
  for (int i = 0; i < n; i++) {
    sg4::ExecPtr op = sg4::this_actor::exec_init(size);
    if (rate != -1.0)
      op->set_bound(rate);
    op->start();
    activities.emplace_back(op);
  }

  // waiting for executions
  for (const auto& act : activities)
    act->wait();
  XBT_INFO("Finished running 2 activities, elapsed %lf", sg4::Engine::get_clock() - start);
}

static void worker()
{
  XBT_INFO("Running 1 tasks (1.5e6) in a 2*(1e6) speed host. Should take 2s (1 (limited by core) + 1 (limited by speed "
           "file)");
  run_activities(1, 1.5e6);

  XBT_INFO("Running 1 tasks (.8e6) in a 2*(1e6) speed host with limited rate (.4e6). Should take 2s (limited by rate)");
  run_activities(1, .8e6, .4e6);

  XBT_INFO("Running 1 tasks (1.1e6) in a 2*(1e6) speed host with limited rate (.6e6). Should take 2s (.6 limited by "
           "rate + .5 limited by speed file)");
  run_activities(1, 1.1e6, .6e6);

  XBT_INFO("Running 2 tasks (1e6) in a 2*(1e6) speed host. Should take 1s");
  run_activities(2, 1e6);

  XBT_INFO("Running 2 tasks (.5e6) in a .5*2*(1e6) speed host. Should take 1s");
  run_activities(2, .5e6);

  XBT_INFO("Running 2 tasks (1.1e6) with limited rate (.6e6). Should take 2s (0.6 limited by rate + 0.5 limited by "
           "speed file)");
  run_activities(2, 1.1e6, .6e6);

  XBT_INFO("Running 2 tasks (.8e6) with limited rate (.4e6). Should take 2s (limited by rate)");
  run_activities(2, .8e6, .4e6);

  XBT_INFO("I'm done. See you!");
}

static void failed_worker()
{
  XBT_INFO("Running a 2 tasks: a small .5e6 and a big 2e6.");
  sg4::ExecPtr ok   = sg4::this_actor::exec_init(.5e6);
  sg4::ExecPtr fail = sg4::this_actor::exec_init(2e6);
  ok->wait();
  XBT_INFO("Finished the small task");
  try {
    XBT_INFO("Waiting big task to finish");
    fail->wait();
  } catch (const simgrid::ForcefulKillException&) {
    XBT_INFO("Unable to finish big task, host went down");
  }
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  /* speed and state file description */
  const char* erin_state_file  = R"(
0 1
1 0
50 1
100 1
)";
  const char* carol_speed_file = R"(
0 1.0
1 0.5
)";
  /* simple platform containing 1 host and 2 disk */
  auto* zone = sg4::create_full_zone("red");
  zone->create_host("erin", 1e6)
      ->set_core_count(2)
      ->set_state_profile(simgrid::kernel::profile::ProfileBuilder::from_string("erin_state", erin_state_file, 0))
      ->seal();
  zone->create_host("carol", 1e6)
      ->set_core_count(2)
      ->set_speed_profile(simgrid::kernel::profile::ProfileBuilder::from_string("carol_speed", carol_speed_file, 1))
      ->seal();
  zone->seal();

  sg4::Actor::create("carol", e.host_by_name("carol"), worker);
  sg4::Actor::create("erin", e.host_by_name("erin"), failed_worker)->set_auto_restart(true);

  e.run();

  XBT_INFO("Simulation time %g", e.get_clock());

  return 0;
}
