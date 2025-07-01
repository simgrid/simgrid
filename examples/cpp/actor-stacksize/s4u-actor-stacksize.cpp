/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This code tests that we can change the stack-size between the actors creation. */

#include <simgrid/s4u.hpp>
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void actor()
{
  XBT_INFO("Hello");
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  auto* tremblay = e.host_by_name("Tremblay");
  // If you don't specify anything, you get the default size (8Mb) or the one passed on the command line
  tremblay->add_actor("actor", actor);

  // You can use set_config(string) to pass a size that will be parsed. That value will be used for any subsequent
  // actors
  sg4::Engine::set_config("contexts/stack-size:16384");
  tremblay->add_actor("actor", actor);
  tremblay->add_actor("actor", actor);

  // You can use set_config(key, value) for the same effect.
  sg4::Engine::set_config("contexts/stack-size", 32 * 1024);
  tremblay->add_actor("actor", actor);
  tremblay->add_actor("actor", actor);

  // Or you can use set_stacksize() before starting the actor to modify only this one
  sg4::Actor::init("actor", tremblay)->set_stacksize(64 * 1024)->start(actor);
  tremblay->add_actor("actor", actor);

  e.run();
  XBT_INFO("Simulation time %g", sg4::Engine::get_clock());

  return 0;
}
