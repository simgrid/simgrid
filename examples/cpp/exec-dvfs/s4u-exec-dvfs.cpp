/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Pstate properties test");

static int dvfs()
{
  double workload = 100E6;
  simgrid::s4u::Host* host = simgrid::s4u::this_actor::get_host();

  unsigned long nb = host->get_pstate_count();
  XBT_INFO("Count of Processor states=%lu", nb);

  XBT_INFO("Current power peak=%f", host->get_speed());

  // Run a Computation
  simgrid::s4u::this_actor::execute(workload);

  double exec_time = simgrid::s4u::Engine::get_clock();
  XBT_INFO("Computation1 duration: %.2f", exec_time);

  // Change power peak
  int new_pstate = 2;

  XBT_INFO("Changing power peak value to %f (at index %d)", host->get_pstate_speed(new_pstate), new_pstate);

  host->set_pstate(new_pstate);

  XBT_INFO("Current power peak=%f", host->get_speed());

  // Run a second Computation
  simgrid::s4u::this_actor::execute(workload);

  exec_time = simgrid::s4u::Engine::get_clock() - exec_time;
  XBT_INFO("Computation2 duration: %.2f", exec_time);

  // Verify that the default pstate is set to 0
  host = simgrid::s4u::Host::by_name_or_null("MyHost2");
  XBT_INFO("Count of Processor states=%lu", host->get_pstate_count());

  XBT_INFO("Current power peak=%f", host->get_speed());
  return 0;
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../platforms/energy_platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("dvfs_test", simgrid::s4u::Host::by_name("MyHost1"), dvfs);
  simgrid::s4u::Actor::create("dvfs_test", simgrid::s4u::Host::by_name("MyHost2"), dvfs);

  e.run();

  XBT_INFO("Total simulation time: %e", simgrid::s4u::Engine::get_clock());

  return 0;
}
