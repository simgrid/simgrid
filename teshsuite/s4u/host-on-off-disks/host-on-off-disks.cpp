/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "simgrid/s4u.hpp"
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

static void io_manager()
{
  XBT_INFO("Starting I/O manager on 'host1'");
  auto disks = sg4::this_actor::get_host()->get_disks();
  /* Write for 2 seconds on each disk */
  sg4::IoPtr write_on_disk1 = disks[0]->write_async(2e6);
  sg4::IoPtr write_on_disk2 = disks[1]->write_async(2e6);
  sg4::ActivitySet pending_activities({write_on_disk1, write_on_disk2});
  pending_activities.wait_all();
  XBT_INFO("All I/Os are complete. Exit now.");
}

static void host_killer()
{
  auto* host1 = sg4::Engine::get_instance()->host_by_name("host1");
  sg4::this_actor::sleep_for(1);
  XBT_INFO("Turning off 'host1'");
  host1->turn_off();
  sg4::this_actor::sleep_for(1);
  XBT_INFO("Turning 'host1' back on");
  host1->turn_on();
}
int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  auto* root  = e.get_netzone_root();
  auto* host1 = root->add_host("host1", 1e9);
  host1->add_disk("disk-1", 1e9, 1e6);
  host1->add_disk("disk-2", 1e9, 1e6);

  auto* host2 = root->add_host("host2", 1e9);

  const auto* link = root->add_link("link", 1e9);
  root->add_route(host1, host2, {link});
  root->seal();

  host1->add_actor("I/O manager", io_manager)->set_auto_restart();
  host2->add_actor("host_killer", host_killer);

  e.run();

  XBT_INFO("Simulation time %g", e.get_clock());

  return 0;
}
