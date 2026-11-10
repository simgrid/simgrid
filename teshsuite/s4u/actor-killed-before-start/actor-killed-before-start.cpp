/* Copyright (c) 2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

// An actor killed before it ever ran must not run at all. Its first simcall would never be answered, so the
// simulation would end in a deadlock, with the dead actor still around when the engine shuts down.

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u test");

int main(int argc, char** argv)
{
  simgrid::s4u::Engine engine(&argc, argv);
  engine.load_platform(argv[1]);
  simgrid::s4u::Host* host = engine.get_all_hosts()[0];

  // Both run in the same step: the actor is created and killed before it gets to run.
  host->add_actor("creator", [host] {
    XBT_INFO("Creating an actor");
    host->add_actor("created", [] {
      XBT_INFO("Running, although killed before it started");
      simgrid::s4u::this_actor::execute(0);
    });
  });
  host->add_actor("killer", [] {
    XBT_INFO("Killing all other actors");
    simgrid::s4u::Actor::kill_all();
  });

  engine.run();
  XBT_INFO("Returned from engine.run()");
  return 0;
}
