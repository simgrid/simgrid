/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example is to show how to use Engine::run(date) to simulate only up to that point in time */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

/* This actor simply executes and waits the activity it got as a parameter. */
static void runner(sg4::ExecPtr activity)
{
  activity->start();
  activity->wait();

  XBT_INFO("Goodbye now!");
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  sg4::Host* fafard = e.host_by_name("Fafard");

  sg4::ExecPtr activity = sg4::this_actor::exec_init(fafard->get_speed() * 10.)->set_host(fafard);
  sg4::Actor::create("runner", fafard, runner, activity);

  while (activity->get_remaining() > 0) {
    XBT_INFO("Remaining amount of flops: %g (%.0f%%)", activity->get_remaining(),
             100 * activity->get_remaining_ratio());
    e.run_until(sg4::Engine::get_clock() + 1);
  }

  XBT_INFO("Simulation time %g", sg4::Engine::get_clock());

  return 0;
}
