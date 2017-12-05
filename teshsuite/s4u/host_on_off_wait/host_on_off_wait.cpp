/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void master()
{
  simgrid::s4u::Host* jupiter = simgrid::s4u::Host::by_name("Jupiter");
  XBT_INFO("Master waiting");
  simgrid::s4u::this_actor::sleep_for(1);

  XBT_INFO("Turning off the worker host");
  jupiter->turnOff();
  XBT_INFO("Master has finished");
}

static void worker()
{
  XBT_INFO("Worker waiting");
  // TODO, This should really be MSG_HOST_FAILURE
  simgrid::s4u::this_actor::sleep_for(5);
  XBT_ERROR("Worker should be off already.");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.loadPlatform(argv[1]);

  simgrid::s4u::Actor::createActor("master", simgrid::s4u::Host::by_name("Tremblay"), master);
  simgrid::s4u::Actor::createActor("worker", simgrid::s4u::Host::by_name("Jupiter"), worker);

  e.run();
  XBT_INFO("Simulation time %g", e.getClock());

  return 0;
}
