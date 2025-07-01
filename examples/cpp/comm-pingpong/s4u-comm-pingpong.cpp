/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_app_pingpong, "Messages specific for this s4u example");

static void pinger(sg4::Mailbox* mailbox_in, sg4::Mailbox* mailbox_out)
{
  XBT_INFO("Ping from mailbox %s to mailbox %s", mailbox_in->get_name().c_str(), mailbox_out->get_name().c_str());

  /* - Do the ping with a 1-Byte payload (latency bound) ... */
  auto* payload = new double(sg4::Engine::get_clock());

  mailbox_out->put(payload, 1);
  /* - ... then wait for the (large) pong */
  auto sender_time = mailbox_in->get_unique<double>();

  double communication_time = sg4::Engine::get_clock() - *sender_time;
  XBT_INFO("Payload received : large communication (bandwidth bound)");
  XBT_INFO("Pong time (bandwidth bound): %.3f", communication_time);
}

static void ponger(sg4::Mailbox* mailbox_in, sg4::Mailbox* mailbox_out)
{
  XBT_INFO("Pong from mailbox %s to mailbox %s", mailbox_in->get_name().c_str(), mailbox_out->get_name().c_str());

  /* - Receive the (small) ping first ....*/
  auto sender_time          = mailbox_in->get_unique<double>();
  double communication_time = sg4::Engine::get_clock() - *sender_time;
  XBT_INFO("Payload received : small communication (latency bound)");
  XBT_INFO("Ping time (latency bound) %f", communication_time);

  /*  - ... Then send a 1GB pong back (bandwidth bound) */
  auto* payload = new double(sg4::Engine::get_clock());
  XBT_INFO("payload = %.3f", *payload);

  mailbox_out->put(payload, 1e9);
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  sg4::Mailbox* mb1 = e.mailbox_by_name_or_create("Mailbox 1");
  sg4::Mailbox* mb2 = e.mailbox_by_name_or_create("Mailbox 2");

  e.host_by_name("Tremblay")->add_actor("pinger", pinger, mb1, mb2);
  e.host_by_name("Jupiter")->add_actor("ponger", ponger, mb2, mb1);

  e.run();

  XBT_INFO("Total simulation time: %.3f", e.get_clock());

  return 0;
}
