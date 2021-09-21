/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void wizard()
{
  const simgrid::s4u::Host* fafard = simgrid::s4u::Host::by_name("Fafard");
  simgrid::s4u::Host* ginette = simgrid::s4u::Host::by_name("Ginette");
  simgrid::s4u::Host* boivin  = simgrid::s4u::Host::by_name("Boivin");

  XBT_INFO("I'm a wizard! I can run an activity on the Ginette host from the Fafard one! Look!");
  simgrid::s4u::ExecPtr exec = simgrid::s4u::this_actor::exec_init(48.492e6);
  exec->set_host(ginette);
  exec->start();
  XBT_INFO("It started. Running 48.492Mf takes exactly one second on Ginette (but not on Fafard).");

  simgrid::s4u::this_actor::sleep_for(0.1);
  XBT_INFO("Loads in flops/s: Boivin=%.0f; Fafard=%.0f; Ginette=%.0f", boivin->get_load(), fafard->get_load(),
           ginette->get_load());

  exec->wait();

  XBT_INFO("Done!");
  XBT_INFO("And now, harder. Start a remote activity on Ginette and move it to Boivin after 0.5 sec");
  exec = simgrid::s4u::this_actor::exec_init(73293500)->set_host(ginette);
  exec->start();

  simgrid::s4u::this_actor::sleep_for(0.5);
  XBT_INFO("Loads before the move: Boivin=%.0f; Fafard=%.0f; Ginette=%.0f", boivin->get_load(), fafard->get_load(),
           ginette->get_load());

  exec->set_host(boivin);

  simgrid::s4u::this_actor::sleep_for(0.1);
  XBT_INFO("Loads after the move: Boivin=%.0f; Fafard=%.0f; Ginette=%.0f", boivin->get_load(), fafard->get_load(),
           ginette->get_load());

  exec->wait();
  XBT_INFO("Done!");

  XBT_INFO("And now, even harder. Start a remote activity on Ginette and turn off the host after 0.5 sec");
  exec = simgrid::s4u::this_actor::exec_init(48.492e6)->set_host(ginette);
  exec->start();

  simgrid::s4u::this_actor::sleep_for(0.5);
  ginette->turn_off();
  try {
    exec->wait();
  } catch (const simgrid::HostFailureException&) {
    XBT_INFO("Execution failed on %s", ginette->get_cname());
  }
  XBT_INFO("Done!");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  simgrid::s4u::Actor::create("test", e.host_by_name("Fafard"), wizard);

  e.run();

  return 0;
}
