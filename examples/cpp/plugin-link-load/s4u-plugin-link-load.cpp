/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/plugins/load.h"
#include "simgrid/s4u.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void sender(const std::string& mailbox, uint64_t msg_size)
{
  auto mbox          = sg4::Mailbox::by_name(mailbox);
  static int payload = 42;
  mbox->put(&payload, msg_size);
}

static void receiver(const std::string& mailbox)
{
  auto mbox = sg4::Mailbox::by_name(mailbox);
  mbox->get<int>();
}

static void run_transfer(sg4::Host* src_host, sg4::Host* dst_host, const std::string& mailbox, unsigned long msg_size)
{
  XBT_INFO("Launching the transfer of %lu bytes", msg_size);
  sg4::Actor::create("sender", src_host, sender, mailbox, msg_size);
  sg4::Actor::create("receiver", dst_host, receiver, mailbox);
}

static void execute_load_test()
{
  auto host0 = sg4::Host::by_name("node-0.simgrid.org");
  auto host1 = sg4::Host::by_name("node-1.simgrid.org");

  sg4::this_actor::sleep_for(1);
  run_transfer(host0, host1, "1", 1000 * 1000 * 1000);

  sg4::this_actor::sleep_for(10);
  run_transfer(host0, host1, "2", 1000 * 1000 * 1000);
  sg4::this_actor::sleep_for(3);
  run_transfer(host0, host1, "3", 1000 * 1000 * 1000);
}

static void show_link_load(const std::string& link_name, const sg4::Link* link)
{
  XBT_INFO("%s link load (cum, avg, min, max): (%g, %g, %g, %g)", link_name.c_str(), sg_link_get_cum_load(link),
           sg_link_get_avg_load(link), sg_link_get_min_instantaneous_load(link),
           sg_link_get_max_instantaneous_load(link));
}

static void monitor()
{
  auto link_backbone = sg4::Link::by_name("cluster0_backbone");
  auto link_host0    = sg4::Link::by_name("cluster0_link_0_UP");
  auto link_host1    = sg4::Link::by_name("cluster0_link_1_DOWN");

  XBT_INFO("Tracking desired links");
  sg_link_load_track(link_backbone);
  sg_link_load_track(link_host0);
  sg_link_load_track(link_host1);

  show_link_load("Backbone", link_backbone);
  while (sg4::Engine::get_clock() < 5) {
    sg4::this_actor::sleep_for(1);
    show_link_load("Backbone", link_backbone);
  }

  XBT_INFO("Untracking the backbone link");
  sg_link_load_untrack(link_backbone);

  show_link_load("Host0_UP", link_host0);
  show_link_load("Host1_UP", link_host1);

  XBT_INFO("Now resetting and probing host links each second.");

  while (sg4::Engine::get_clock() < 29) {
    sg_link_load_reset(link_host0);
    sg_link_load_reset(link_host1);

    sg4::this_actor::sleep_for(1);

    show_link_load("Host0_UP", link_host0);
    show_link_load("Host1_UP", link_host1);
  }
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  sg_link_load_plugin_init();

  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s ../platforms/energy_platform.xml\n", argv[0], argv[0]);
  e.load_platform(argv[1]);

  sg4::Actor::create("load_test", e.host_by_name("node-42.simgrid.org"), execute_load_test);
  sg4::Actor::create("monitor", e.host_by_name("node-51.simgrid.org"), monitor);

  e.run();

  XBT_INFO("Total simulation time: %.2f", sg4::Engine::get_clock());

  return 0;
}
