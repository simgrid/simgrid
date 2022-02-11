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

static void server()
{
  const int* received1 = nullptr;
  const int* received2 = nullptr;

  received1 = simgrid::s4u::Mailbox::by_name("mymailbox")->get<int>();
  long val1 = *received1;
  delete received1;

  received2 = simgrid::s4u::Mailbox::by_name("mymailbox")->get<int>();
  long val2 = *received2;
  delete received2;

  XBT_INFO("First pair received: %ld %ld", val1, val2);

  MC_assert(std::min(val1, val2) == 1); // if the two messages of the second client arrive first, this is violated.

  received1 = simgrid::s4u::Mailbox::by_name("mymailbox")->get<int>();
  val1      = *received1;
  delete received1;

  received2 = simgrid::s4u::Mailbox::by_name("mymailbox")->get<int>();
  val2      = *received2;
  delete received2;

  XBT_INFO("Second pair received: %ld %ld", val1, val2);
}

static void client(int id)
{
  auto* payload1 = new int(id);
  auto* payload2 = new int(id);

  simgrid::s4u::Mailbox::by_name("mymailbox")->put(payload1, 10000);
  simgrid::s4u::Mailbox::by_name("mymailbox")->put(payload2, 10000);
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("server", e.host_by_name("HostA"), server);
  simgrid::s4u::Actor::create("client", e.host_by_name("HostB"), client, 1);
  simgrid::s4u::Actor::create("client", e.host_by_name("HostC"), client, 2);

  e.run();
  return 0;
}
