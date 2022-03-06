/* Copyright (c) 2017-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

/* This example demonstrates how to use wifi links in SimGrid. Most of the interesting things happen in the
 * corresponding XML file: examples/platforms/wifi.xml
 */

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_network_wifi, "Messages specific for this s4u example");
namespace sg4 = simgrid::s4u;

static void sender(sg4::Mailbox* mailbox, int data_size)
{
  XBT_INFO("Send a message to the other station.");
  static std::string message = "message";
  mailbox->put(&message, data_size);
  XBT_INFO("Done.");
}
static void receiver(sg4::Mailbox* mailbox)
{
  XBT_INFO("Wait for a message.");
  mailbox->get<std::string>();
  XBT_INFO("Done.");
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  xbt_assert(argc > 1, "Usage: %s platform_file\n\tExample: %s platform.xml deployment.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]);

  /* Exchange a message between the 2 stations */
  auto mailbox  = sg4::Mailbox::by_name("mailbox");
  auto station1 = e.host_by_name("Station 1");
  auto station2 = e.host_by_name("Station 2");
  sg4::Actor::create("sender", station1, sender, mailbox, 1e7);
  sg4::Actor::create("receiver", station2, receiver, mailbox);

  /* Declare that the stations are not at the same distance from their AP */
  auto ap = e.link_by_name("AP1");
  ap->set_host_wifi_rate(station1, 1); // The host "Station 1" uses the second level of bandwidths on that AP
  ap->set_host_wifi_rate(station2, 0); // This is perfectly useless as level 0 is used by default

  e.run();

  return 0;
}
