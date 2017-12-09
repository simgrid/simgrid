/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "simgrid/forward.h"
#include "simgrid/s4u/forward.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void test(double computation_amount, double priority)
{
  XBT_INFO("Hello! Execute %g flops with priority %g", computation_amount, priority);
  simgrid::s4u::ExecPtr activity = simgrid::s4u::this_actor::exec_init(computation_amount);
  activity->setPriority(priority);
  activity->start();
  activity->wait();

  XBT_INFO("Goodbye now!");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.loadPlatform(argv[1]);
  simgrid::s4u::Actor::createActor("test", simgrid::s4u::Host::by_name("Fafard"), test, 7.6296e+07, 1.0);
  simgrid::s4u::Actor::createActor("test", simgrid::s4u::Host::by_name("Fafard"), test, 7.6296e+07, 2.0);

  e.run();

  XBT_INFO("Simulation time %g", e.getClock());

  return 0;
}
