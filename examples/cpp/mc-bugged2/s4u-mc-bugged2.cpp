/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/******************** Non-deterministic message ordering  *********************/
/* Server assumes a fixed order in the reception of messages from its clients */
/* which is incorrect because the message ordering is non-deterministic       */
/******************************************************************************/

#include <simgrid/modelchecker.h>
#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(example, "this example");
namespace sg4 = simgrid::s4u;

static void server()
{
  auto received1 = sg4::Mailbox::by_name("mymailbox")->get_unique<int>();
  long val1 = *received1;

  auto received2 = sg4::Mailbox::by_name("mymailbox")->get_unique<int>();
  long val2 = *received2;

  XBT_INFO("First pair received: %ld %ld", val1, val2);

  MC_assert(std::min(val1, val2) == 1); // if the two messages of the second client arrive first, this is violated.

  received1 = sg4::Mailbox::by_name("mymailbox")->get_unique<int>();
  val1      = *received1;

  received2 = sg4::Mailbox::by_name("mymailbox")->get_unique<int>();
  val2      = *received2;

  XBT_INFO("Second pair received: %ld %ld", val1, val2);
}

static void client(int id)
{
  auto* payload1 = new int(id);
  auto* payload2 = new int(id);

  sg4::Mailbox::by_name("mymailbox")->put(payload1, 10000);
  sg4::Mailbox::by_name("mymailbox")->put(payload2, 10000);
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  e.load_platform(argv[1]);

  sg4::Actor::create("server", e.host_by_name("HostA"), server);
  sg4::Actor::create("client", e.host_by_name("HostB"), client, 1);
  sg4::Actor::create("client", e.host_by_name("HostC"), client, 2);

  e.run();
  return 0;
}
