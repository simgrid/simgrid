/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/******************** Non-deterministic message ordering  *********************/
/* Server assumes a fixed order in the reception of messages from its clients */
/* which is incorrect because the message ordering is non-deterministic       */
/******************************************************************************/

#include <simgrid/modelchecker.h>
#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(example, "this example");

static void server()
{
  void* received1 = nullptr;
  void* received2 = nullptr;

  received1 = simgrid::s4u::Mailbox::by_name("mymailbox")->get();
  long val1 = *(static_cast<int*>(received1));
  received1 = nullptr;
  XBT_INFO("Received %ld", val1);

  received2 = simgrid::s4u::Mailbox::by_name("mymailbox")->get();
  long val2 = *(static_cast<int*>(received2));
  received2 = nullptr;
  XBT_INFO("Received %ld", val2);

  MC_assert(std::min(val1, val2) == 1);

  received1 = simgrid::s4u::Mailbox::by_name("mymailbox")->get();
  val1      = *(static_cast<int*>(received1));
  XBT_INFO("Received %ld", val1);

  received2 = simgrid::s4u::Mailbox::by_name("mymailbox")->get();
  val2      = *(static_cast<int*>(received2));
  XBT_INFO("Received %ld", val2);

  XBT_INFO("OK");
}

static void client(int id)
{
  auto* payload1 = new int(id);
  auto* payload2 = new int(id);

  XBT_INFO("Send %d", id);
  simgrid::s4u::Mailbox::by_name("mymailbox")->put(payload1, 10000);

  XBT_INFO("Send %d", id);
  simgrid::s4u::Mailbox::by_name("mymailbox")->put(payload2, 10000);
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("server", simgrid::s4u::Host::by_name("HostA"), server);
  simgrid::s4u::Actor::create("client", simgrid::s4u::Host::by_name("HostB"), client, 1);
  simgrid::s4u::Actor::create("client", simgrid::s4u::Host::by_name("HostC"), client, 2);

  e.run();
  return 0;
}
