/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void worker()
{
  // Define an amount of work that should take 1 second to execute.
  double computation_amount = sg4::this_actor::get_host()->get_speed();

  // Create an unassigned activity and start it
  sg4::ExecPtr exec = sg4::Exec::init()->set_flops_amount(computation_amount)->set_name("exec")->vetoable_start();

  // Wait for a while
  sg4::this_actor::sleep_for(10);

  // Assign the activity to the current host. This triggers its start, then waits for it completion.
  exec->set_host(sg4::Host::current())->wait();
  XBT_INFO("Exec '%s' is complete", exec->get_cname());
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  sg4::Actor::create("worker", e.host_by_name("Fafard"), worker);

  e.run();

  XBT_INFO("Simulation time %g", sg4::Engine::get_clock());

  return 0;
}
