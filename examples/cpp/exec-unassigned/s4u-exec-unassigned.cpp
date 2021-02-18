/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void worker()
{
  // Define an amount of work that should take 1 second to execute.
  double computation_amount = simgrid::s4u::this_actor::get_host()->get_speed();

  // Create an unassigned activity and start it
  simgrid::s4u::ExecPtr exec =
      simgrid::s4u::Exec::init()->set_flops_amount(computation_amount)->set_name("exec")->vetoable_start();

  // Wait for a while
  simgrid::s4u::this_actor::sleep_for(10);

  // Assign the activity to the current host. This triggers its start, then waits for it completion.
  exec->set_host(simgrid::s4u::Host::current())->wait();
  XBT_INFO("Exec '%s' is complete", exec->get_cname());
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("worker", simgrid::s4u::Host::by_name("Fafard"), worker);

  e.run();

  XBT_INFO("Simulation time %g", simgrid::s4u::Engine::get_clock());

  return 0;
}
