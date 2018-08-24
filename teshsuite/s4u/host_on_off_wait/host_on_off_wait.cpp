/* Copyright (c) 2010-2018. The SimGrid Team. All rights reserved.          */

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
  jupiter->turn_off();
  XBT_INFO("Master has finished");
}

static void worker()
{
  XBT_INFO("Worker waiting");
  try {
    simgrid::s4u::this_actor::sleep_for(5);
  } catch (xbt_ex& e) {
    if (e.category == host_error) {
      XBT_INFO("The host has died ... as expected.");
    } else {
      XBT_ERROR("An unexpected exception has been raised.");
      throw;
    }
  }
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("master", simgrid::s4u::Host::by_name("Tremblay"), master);
  simgrid::s4u::Actor::create("worker", simgrid::s4u::Host::by_name("Jupiter"), worker);

  e.run();
  XBT_INFO("Simulation time %g", e.get_clock());

  return 0;
}
