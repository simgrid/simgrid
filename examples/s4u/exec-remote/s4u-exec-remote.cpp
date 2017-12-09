/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "simgrid/forward.h"
#include "simgrid/s4u/forward.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void wizard()
{
  simgrid::s4u::Host* fafard  = simgrid::s4u::Host::by_name("Fafard");
  simgrid::s4u::Host* ginette = simgrid::s4u::Host::by_name("Ginette");

  XBT_INFO("I'm a wizard! I can run a task on the Fafard host from the Ginette one! Look!");
  simgrid::s4u::ExecPtr activity = simgrid::s4u::this_actor::exec_init(48.492e6);
  activity->setHost(ginette);
  activity->start();
  XBT_INFO("It started. Running 48.492Mf takes exactly one second on Ginette (but not on Fafard).");

  simgrid::s4u::this_actor::sleep_for(0.1);
  XBT_INFO("Load on Fafard: %e flops/s; Load on Ginette: %e flops/s.", fafard->getLoad(), ginette->getLoad());

  activity->wait();

  XBT_INFO("Done!");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.loadPlatform(argv[1]);
  simgrid::s4u::Actor::createActor("test", simgrid::s4u::Host::by_name("Fafard"), wizard);

  e.run();

  return 0;
}
