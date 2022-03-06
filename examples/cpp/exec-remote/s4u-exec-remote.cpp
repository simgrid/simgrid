/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void wizard()
{
  const sg4::Host* fafard = sg4::Host::by_name("Fafard");
  sg4::Host* ginette      = sg4::Host::by_name("Ginette");
  sg4::Host* boivin       = sg4::Host::by_name("Boivin");

  XBT_INFO("I'm a wizard! I can run an activity on the Ginette host from the Fafard one! Look!");
  sg4::ExecPtr exec = sg4::this_actor::exec_init(48.492e6);
  exec->set_host(ginette);
  exec->start();
  XBT_INFO("It started. Running 48.492Mf takes exactly one second on Ginette (but not on Fafard).");

  sg4::this_actor::sleep_for(0.1);
  XBT_INFO("Loads in flops/s: Boivin=%.0f; Fafard=%.0f; Ginette=%.0f", boivin->get_load(), fafard->get_load(),
           ginette->get_load());

  exec->wait();

  XBT_INFO("Done!");
  XBT_INFO("And now, harder. Start a remote activity on Ginette and move it to Boivin after 0.5 sec");
  exec = sg4::this_actor::exec_init(73293500)->set_host(ginette);
  exec->start();

  sg4::this_actor::sleep_for(0.5);
  XBT_INFO("Loads before the move: Boivin=%.0f; Fafard=%.0f; Ginette=%.0f", boivin->get_load(), fafard->get_load(),
           ginette->get_load());

  exec->set_host(boivin);

  sg4::this_actor::sleep_for(0.1);
  XBT_INFO("Loads after the move: Boivin=%.0f; Fafard=%.0f; Ginette=%.0f", boivin->get_load(), fafard->get_load(),
           ginette->get_load());

  exec->wait();
  XBT_INFO("Done!");

  XBT_INFO("And now, even harder. Start a remote activity on Ginette and turn off the host after 0.5 sec");
  exec = sg4::this_actor::exec_init(48.492e6)->set_host(ginette);
  exec->start();

  sg4::this_actor::sleep_for(0.5);
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
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);
  sg4::Actor::create("test", e.host_by_name("Fafard"), wizard);

  e.run();

  return 0;
}
