/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_app_pingpong, "Messages specific for this s4u example");

static void pinger(simgrid::s4u::Mailbox* mailbox_in, simgrid::s4u::Mailbox* mailbox_out)
{
  XBT_INFO("Ping from mailbox %s to mailbox %s", mailbox_in->get_name().c_str(), mailbox_out->get_name().c_str());

  /* - Do the ping with a 1-Byte payload (latency bound) ... */
  auto* payload = new double(simgrid::s4u::Engine::get_clock());

  mailbox_out->put(payload, 1);
  /* - ... then wait for the (large) pong */
  const auto* sender_time = mailbox_in->get<double>();

  double communication_time = simgrid::s4u::Engine::get_clock() - *sender_time;
  XBT_INFO("Payload received : large communication (bandwidth bound)");
  XBT_INFO("Pong time (bandwidth bound): %.3f", communication_time);
  delete sender_time;
}

static void ponger(simgrid::s4u::Mailbox* mailbox_in, simgrid::s4u::Mailbox* mailbox_out)
{
  XBT_INFO("Pong from mailbox %s to mailbox %s", mailbox_in->get_name().c_str(), mailbox_out->get_name().c_str());

  /* - Receive the (small) ping first ....*/
  const auto* sender_time   = mailbox_in->get<double>();
  double communication_time = simgrid::s4u::Engine::get_clock() - *sender_time;
  XBT_INFO("Payload received : small communication (latency bound)");
  XBT_INFO("Ping time (latency bound) %f", communication_time);
  delete sender_time;

  /*  - ... Then send a 1GB pong back (bandwidth bound) */
  auto* payload = new double(simgrid::s4u::Engine::get_clock());
  XBT_INFO("payload = %.3f", *payload);

  mailbox_out->put(payload, 1e9);
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  simgrid::s4u::Mailbox* mb1 = simgrid::s4u::Mailbox::by_name("Mailbox 1");
  simgrid::s4u::Mailbox* mb2 = simgrid::s4u::Mailbox::by_name("Mailbox 2");

  simgrid::s4u::Actor::create("pinger", simgrid::s4u::Host::by_name("Tremblay"), pinger, mb1, mb2);
  simgrid::s4u::Actor::create("ponger", simgrid::s4u::Host::by_name("Jupiter"), ponger, mb2, mb1);

  e.run();

  XBT_INFO("Total simulation time: %.3f", e.get_clock());

  return 0;
}
