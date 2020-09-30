/* Copyright (c) 2017-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"

/* This example demonstrates how to use wifi links in SimGrid. Most of the interesting things happen in the
 * corresponding XML file.
 */

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_network_wifi, "Messages specific for this s4u example");

static void sender(simgrid::s4u::Mailbox* mailbox, int data_size)
{
  XBT_INFO("Send a message to the other station.");
  static char message[] = "message";
  mailbox->put(message, data_size);
  XBT_INFO("Done.");
}
static void receiver(simgrid::s4u::Mailbox* mailbox)
{
  XBT_INFO("Wait for a message.");
  mailbox->get();
  XBT_INFO("Done.");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  xbt_assert(argc > 1, "Usage: %s platform_file\n\tExample: %s platform.xml deployment.xml\n", argv[0], argv[0]);

  e.load_platform(argv[1]);

  auto mailbox  = simgrid::s4u::Mailbox::by_name("mailbox");
  auto station1 = simgrid::s4u::Host::by_name("Station 1");
  auto station2 = simgrid::s4u::Host::by_name("Station 2");
  simgrid::s4u::Actor::create("sender", station1, sender, mailbox, 1e7);
  simgrid::s4u::Actor::create("receiver", station2, receiver, mailbox);

  auto ap = simgrid::s4u::Link::by_name("AP1");
  ap->set_host_wifi_rate(station1, 1); // The host "Station 1" uses the second level of bandwidths on that AP
  ap->set_host_wifi_rate(station2, 0); // This is perfectly useless as level 0 is used by default

  e.run();

  return 0;
}
