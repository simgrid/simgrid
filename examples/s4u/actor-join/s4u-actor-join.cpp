/* Copyright (c) 2017-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void sleeper()
{
  XBT_INFO("Sleeper started");
  sg4::this_actor::sleep_for(3);
  XBT_INFO("I'm done. See you!");
}

static void master()
{
  sg4::ActorPtr actor;

  XBT_INFO("Start sleeper");
  actor = sg4::Actor::create("sleeper from master", sg4::Host::current(), sleeper);
  XBT_INFO("Join the sleeper (timeout 2)");
  actor->join(2);

  XBT_INFO("Start sleeper");
  actor = sg4::Actor::create("sleeper from master", sg4::Host::current(), sleeper);
  XBT_INFO("Join the sleeper (timeout 4)");
  actor->join(4);

  XBT_INFO("Start sleeper");
  actor = sg4::Actor::create("sleeper from master", sg4::Host::current(), sleeper);
  XBT_INFO("Join the sleeper (timeout 2)");
  actor->join(2);

  XBT_INFO("Start sleeper");
  actor = sg4::Actor::create("sleeper from master", sg4::Host::current(), sleeper);
  XBT_INFO("Waiting 4");
  sg4::this_actor::sleep_for(4);
  XBT_INFO("Join the sleeper after its end (timeout 1)");
  actor->join(1);

  XBT_INFO("Goodbye now!");

  sg4::this_actor::sleep_for(1);

  XBT_INFO("Goodbye now!");
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../platforms/small_platform.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]);

  sg4::Actor::create("master", sg4::Host::by_name("Tremblay"), master);

  e.run();

  XBT_INFO("Simulation time %g", sg4::Engine::get_clock());

  return 0;
}
