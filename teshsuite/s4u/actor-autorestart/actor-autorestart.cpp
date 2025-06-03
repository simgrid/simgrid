/* Copyright (c) 2017-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void dummy()
{
  XBT_INFO("I start");
  sg4::this_actor::sleep_for(200);
  XBT_INFO("I stop");
}

static void dummy_daemon()
{
  while (sg4::this_actor::get_host()->is_on()) {
    XBT_INFO("Hello from the infinite loop");
    sg4::this_actor::sleep_for(80.0);
  }
}

static void autostart()
{
  sg4::Host* host = sg4::Host::by_name("Fafard");

  XBT_INFO("starting a dummy process on %s", host->get_cname());
  sg4::ActorPtr dummy_actor = host->add_actor("Dummy", dummy);
  dummy_actor->on_exit([](bool failed) { XBT_INFO("Dummy actor %s.", failed ? "failed" : "terminating"); });
  dummy_actor->set_auto_restart(true);
  dummy_actor->on_exit([](bool) { XBT_INFO("On_exit callback set after autorestart"); });

  XBT_INFO("starting a daemon process on %s", host->get_cname());
  sg4::ActorPtr daemon_actor = host->add_actor("Daemon", dummy_daemon);
  daemon_actor->on_exit([](bool failed) { XBT_INFO("Daemon actor %s.", failed ? "failed" : "terminating"); });
  daemon_actor->daemonize()->set_auto_restart(true);
  daemon_actor->on_exit([](bool) { XBT_INFO("On_exit callback set after autorestart"); });

  sg4::this_actor::sleep_for(50);

  XBT_INFO("powering off %s", host->get_cname());
  host->turn_off();

  sg4::this_actor::sleep_for(10);

  XBT_INFO("powering on %s", host->get_cname());
  host->turn_on();
  sg4::this_actor::sleep_for(200);
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  e.host_by_name("Tremblay")->add_actor("Autostart", autostart);

  e.run();
  XBT_INFO("Simulation time %g", e.get_clock());

  return 0;
}
