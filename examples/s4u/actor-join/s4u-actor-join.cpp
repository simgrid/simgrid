/* Copyright (c) 2017-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void sleeper()
{
  XBT_INFO("Sleeper started");
  simgrid::s4u::this_actor::sleep_for(3);
  XBT_INFO("I'm done. See you!");
}

static void master()
{
  simgrid::s4u::ActorPtr actor;

  XBT_INFO("Start sleeper");
  actor = simgrid::s4u::Actor::create("sleeper from master", simgrid::s4u::Host::current(), sleeper);
  XBT_INFO("Join the sleeper (timeout 2)");
  actor->join(2);

  XBT_INFO("Start sleeper");
  actor = simgrid::s4u::Actor::create("sleeper from master", simgrid::s4u::Host::current(), sleeper);
  XBT_INFO("Join the sleeper (timeout 4)");
  actor->join(4);

  XBT_INFO("Start sleeper");
  actor = simgrid::s4u::Actor::create("sleeper from master", simgrid::s4u::Host::current(), sleeper);
  XBT_INFO("Join the sleeper (timeout 2)");
  actor->join(2);

  XBT_INFO("Start sleeper");
  actor = simgrid::s4u::Actor::create("sleeper from master", simgrid::s4u::Host::current(), sleeper);
  XBT_INFO("Waiting 4");
  simgrid::s4u::this_actor::sleep_for(4);
  XBT_INFO("Join the sleeper after its end (timeout 1)");
  actor->join(1);

  XBT_INFO("Goodbye now!");

  simgrid::s4u::this_actor::sleep_for(1);

  XBT_INFO("Goodbye now!");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../platforms/small_platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("master", simgrid::s4u::Host::by_name("Tremblay"), master);

  e.run();

  XBT_INFO("Simulation time %g", simgrid::s4u::Engine::get_clock());

  return 0;
}
