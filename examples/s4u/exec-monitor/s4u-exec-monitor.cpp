/* Copyright (c) 2017. The SimGrid Team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "simgrid/forward.h"
#include "simgrid/s4u/VirtualMachine.hpp"
#include "simgrid/s4u/forward.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void monitor(simgrid::s4u::ExecPtr activity)
{
  while (not activity->test()) {
    XBT_INFO("activity remaining duration: %g (%.0f%%)", activity->getRemains(), 100 * activity->getRemainingRatio());
    simgrid::s4u::this_actor::sleep_for(5);
  }
  XBT_INFO("My task is over.");
}

static void executor()
{
  XBT_INFO("Create one monitored task, and wait for it");
  simgrid::s4u::ExecPtr activity = simgrid::s4u::this_actor::exec_async(1e9);
  simgrid::s4u::Actor::createActor("monitor 1", simgrid::s4u::Host::by_name("Tremblay"), monitor, activity);
  activity->wait(); // This blocks until the activity is over
  XBT_INFO("The monitored task is over. Let's start 3 of them now.");
  simgrid::s4u::Actor::createActor("monitor 2", simgrid::s4u::Host::by_name("Jupiter"), monitor,
                                   simgrid::s4u::this_actor::exec_async(1e9));
  simgrid::s4u::Actor::createActor("monitor 3", simgrid::s4u::Host::by_name("Ginette"), monitor,
                                   simgrid::s4u::this_actor::exec_async(1e9));
  simgrid::s4u::Actor::createActor("monitor 4", simgrid::s4u::Host::by_name("Bourassa"), monitor,
                                   simgrid::s4u::this_actor::exec_async(1e9));
  XBT_INFO("All activities are started; finish now");
  // Waiting execution activities is not mandatory: they go to completion once started

  // No memory is leaked here: activities are automatically refcounted, thanks to C++ smart pointers
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.loadPlatform(argv[1]);

  simgrid::s4u::Actor::createActor("executor", simgrid::s4u::Host::by_name("Fafard"), executor);

  e.run();

  return 0;
}
