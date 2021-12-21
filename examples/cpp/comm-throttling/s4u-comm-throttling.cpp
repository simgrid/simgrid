/* Copyright (c) 2007-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u.hpp>
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_comm_throttling, "Messages specific for this s4u example");

static void sender(sg4::Mailbox* mailbox)
{
  XBT_INFO("Send at full bandwidth");

  /* - First send a 2.5e8 Bytes payload at full bandwidth (1.25e8 Bps) */
  auto* payload = new double(sg4::Engine::get_clock());
  mailbox->put(payload, 2.5e8);

  XBT_INFO("Throttle the bandwidth at the Comm level");
  /* - ... then send it again but throttle the Comm */
  payload = new double(sg4::Engine::get_clock());
  /* get a handler on the comm first */
  sg4::CommPtr comm = mailbox->put_init(payload, 2.5e8);

  /* let throttle the communication. It amounts to set the rate of the comm to half the nominal bandwidth of the link,
   * i.e., 1.25e8 / 2. This second communication will thus take approximately twice as long as the first one*/
  comm->set_rate(1.25e8 / 2)->wait();
}

static void receiver(sg4::Mailbox* mailbox)
{
  /* - Receive the first payload sent at full bandwidth */
  auto sender_time          = mailbox->get_unique<double>();
  double communication_time = sg4::Engine::get_clock() - *sender_time;
  XBT_INFO("Payload received (full bandwidth) in %f seconds", communication_time);

  /*  - ... Then receive the second payload sent with a throttled Comm */
  sender_time        = mailbox->get_unique<double>();
  communication_time = sg4::Engine::get_clock() - *sender_time;
  XBT_INFO("Payload received (throttled) in %f seconds", communication_time);
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  sg4::Mailbox* mbox = e.mailbox_by_name_or_create("Mailbox");

  sg4::Actor::create("sender", e.host_by_name("node-0.simgrid.org"), sender, mbox);
  sg4::Actor::create("receiver", e.host_by_name("node-1.simgrid.org"), receiver, mbox);

  e.run();

  return 0;
}
