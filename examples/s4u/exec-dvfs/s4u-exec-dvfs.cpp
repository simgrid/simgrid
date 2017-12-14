/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Pstate properties test");

static int dvfs()
{
  double workload = 100E6;
  simgrid::s4u::Host* host = simgrid::s4u::this_actor::getHost();

  int nb = host->getPstatesCount();
  XBT_INFO("Count of Processor states=%d", nb);

  XBT_INFO("Current power peak=%f", host->getSpeed());

  // Run a task
  simgrid::s4u::this_actor::execute(workload);

  double task_time = simgrid::s4u::Engine::getClock();
  XBT_INFO("Task1 simulation time: %e", task_time);

  // Change power peak
  int new_pstate = 2;

  XBT_INFO("Changing power peak value to %f (at index %d)", host->getPstateSpeed(new_pstate), new_pstate);

  host->setPstate(new_pstate);

  XBT_INFO("Current power peak=%f", host->getSpeed());

  // Run a second task
  simgrid::s4u::this_actor::execute(workload);

  task_time = simgrid::s4u::Engine::getClock() - task_time;
  XBT_INFO("Task2 simulation time: %e", task_time);

  // Verify that the default pstate is set to 0
  host = simgrid::s4u::Host::by_name_or_null("MyHost2");
  XBT_INFO("Count of Processor states=%d", host->getPstatesCount());

  XBT_INFO("Current power peak=%f", host->getSpeed());
  return 0;
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  e.loadPlatform(argv[1]);

  simgrid::s4u::Actor::createActor("dvfs_test", simgrid::s4u::Host::by_name("MyHost1"), dvfs);
  simgrid::s4u::Actor::createActor("dvfs_test", simgrid::s4u::Host::by_name("MyHost2"), dvfs);

  e.run();

  XBT_INFO("Total simulation time: %e", e.getClock());

  return 0;
}
