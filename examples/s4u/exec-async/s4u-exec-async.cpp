/* Copyright (c) 2007-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void test(double computation_amount, double priority)
{
  XBT_INFO("Hello! Execute %g flops with priority %g", computation_amount, priority);
  simgrid::s4u::ExecPtr activity = simgrid::s4u::this_actor::exec_init(computation_amount);
  activity->set_priority(priority);
  activity->start();
  activity->wait();

  XBT_INFO("Goodbye now!");
}

static void test_cancel(double computation_amount)
{
  XBT_INFO("Hello! Execute %g flops, should take 1 second", computation_amount);
  simgrid::s4u::ExecPtr activity = simgrid::s4u::this_actor::exec_async(computation_amount);
  simgrid::s4u::this_actor::sleep_for(0.5);
  XBT_INFO("I changed my mind, cancel!");
  activity->cancel();

  XBT_INFO("Goodbye now!");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  simgrid::s4u::Actor::create("test", simgrid::s4u::Host::by_name("Fafard"), test, 7.6296e+07, 1.0);
  simgrid::s4u::Actor::create("test", simgrid::s4u::Host::by_name("Fafard"), test, 7.6296e+07, 2.0);
  simgrid::s4u::Actor::create("test_cancel", simgrid::s4u::Host::by_name("Boivin"), test_cancel, 98.095e+07);

  e.run();

  XBT_INFO("Simulation time %g", e.get_clock());

  return 0;
}
