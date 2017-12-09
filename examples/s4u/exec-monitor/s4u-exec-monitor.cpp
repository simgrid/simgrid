/* Copyright (c) 2014-2015, 2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "simgrid/forward.h"
#include "simgrid/s4u/VirtualMachine.hpp"
#include "simgrid/s4u/forward.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

simgrid::s4u::ExecPtr activity;

static void executor()
{
  const char* actor_name = simgrid::s4u::this_actor::getCname();
  const char* host_name  = simgrid::s4u::Host::current()->getCname();

  double clock_begin = simgrid::s4u::Engine::getClock();
  XBT_INFO("%s:%s task 1 created %g", host_name, actor_name, clock_begin);
  activity = simgrid::s4u::Actor::self()->exec_async(1e9);
  activity->wait();
  double clock_end = simgrid::s4u::Engine::getClock();

  XBT_INFO("%s:%s task 1 executed %g", host_name, actor_name, clock_end - clock_begin);

  activity = nullptr;

  simgrid::s4u::this_actor::sleep_for(1);

  clock_begin = simgrid::s4u::Engine::getClock();
  XBT_INFO("%s:%s task 2 created %g", host_name, actor_name, clock_begin);
  activity = simgrid::s4u::Actor::self()->exec_async(1e10);
  activity->wait();
  clock_end = simgrid::s4u::Engine::getClock();

  XBT_INFO("%s:%s task 2 executed %g", host_name, actor_name, clock_end - clock_begin);
}

static void monitor()
{
  simgrid::s4u::Actor::createActor("compute", simgrid::s4u::Host::by_name("Fafard"), executor);

  while (simgrid::s4u::Engine::getClock() < 100) {
    if (activity)
      XBT_INFO("activity remaining duration: %g", activity->getRemains());
    simgrid::s4u::this_actor::sleep_for(1);
  }

  simgrid::s4u::this_actor::sleep_for(10000);
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.loadPlatform(argv[1]); /* - Load the platform description */

  simgrid::s4u::Actor::createActor("master_", simgrid::s4u::Host::by_name("Fafard"), monitor);

  e.run();

  XBT_INFO("Simulation time %g", e.getClock());

  return 0;
}
