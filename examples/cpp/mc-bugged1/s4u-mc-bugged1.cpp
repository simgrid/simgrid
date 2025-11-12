/* Copyright (c) 2010-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/******************** Non-deterministic message ordering  *********************/
/* Server assumes a fixed order in the reception of messages from its clients */
/* which is incorrect because the message ordering is non-deterministic       */
/******************************************************************************/

#include <simgrid/modelchecker.h>
#include <simgrid/s4u.hpp>

constexpr int N = 3;

XBT_LOG_NEW_DEFAULT_CATEGORY(example, "this example");
namespace sg4 = simgrid::s4u;

static void server()
{
  std::unique_ptr<int> received;
  int count     = 0;
  while (count < N) {
    received.reset();
    received = sg4::Mailbox::by_name("mymailbox")->get_unique<int>();
    count++;
  }
  int value_got = *received;
  MC_assert(value_got == 3);

  XBT_INFO("OK");
}

static void client(int id)
{
  auto* payload = new int(id);
  sg4::Mailbox::by_name("mymailbox")->put(payload, 10000);

  XBT_INFO("Sent!");
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  e.load_platform(argv[1]);

  e.host_by_name("HostA")->add_actor("server", server);
  e.host_by_name("HostB")->add_actor("client", client, 1);
  e.host_by_name("HostC")->add_actor("client", client, 2);
  e.host_by_name("HostD")->add_actor("client", client, 3);

  e.run();
  return 0;
}
