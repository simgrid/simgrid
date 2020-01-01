/* Copyright (c) 2017-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void dummy()
{
  XBT_INFO("I start");
    simgrid::s4u::this_actor::sleep_for(200);
    XBT_INFO("I stop");
}

static void dummy_daemon()
{
  simgrid::s4u::Actor::self()->daemonize();
  while (simgrid::s4u::this_actor::get_host()->is_on()) {
    XBT_INFO("Hello from the infinite loop");
    simgrid::s4u::this_actor::sleep_for(80.0);
  }
}

static void autostart()
{
  simgrid::s4u::Host* host = simgrid::s4u::Host::by_name("Fafard");

  XBT_INFO("starting a dummy process on %s", host->get_cname());
  simgrid::s4u::ActorPtr dummy_actor = simgrid::s4u::Actor::create("Dummy", host, dummy);
  dummy_actor->on_exit([](bool) { XBT_INFO("On_exit callback set before autorestart"); });
  dummy_actor->set_auto_restart(true);
  dummy_actor->on_exit([](bool) { XBT_INFO("On_exit callback set after autorestart"); });

  XBT_INFO("starting a daemon process on %s", host->get_cname());
  simgrid::s4u::ActorPtr daemon_actor = simgrid::s4u::Actor::create("Daemon", host, dummy_daemon);
  daemon_actor->on_exit([](bool) { XBT_INFO("On_exit callback set before autorestart"); });
  daemon_actor->set_auto_restart(true);
  daemon_actor->on_exit([](bool) { XBT_INFO("On_exit callback set after autorestart"); });

  simgrid::s4u::this_actor::sleep_for(50);

  XBT_INFO("powering off %s", host->get_cname());
  host->turn_off();

  simgrid::s4u::this_actor::sleep_for(10);

  XBT_INFO("powering on %s", host->get_cname());
  host->turn_on();
  simgrid::s4u::this_actor::sleep_for(200);
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("Autostart", simgrid::s4u::Host::by_name("Tremblay"), autostart);

  e.run();
  XBT_INFO("Simulation time %g", e.get_clock());

  return 0;
}
