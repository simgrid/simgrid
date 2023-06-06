/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/******************** Non-deterministic message ordering  *********************/
/* Server assumes a fixed order in the reception of messages from its clients */
/* which is incorrect because the message ordering is non-deterministic       */
/******************************************************************************/

#include <simgrid/s4u.hpp>
namespace sg4 = simgrid::s4u;

constexpr int N = 3;

XBT_LOG_NEW_DEFAULT_CATEGORY(example, "this example");

static void server()
{
  auto* mb      = sg4::Mailbox::by_name("mymailbox");
  int value_got = -1;
  for (int count = 0; count < N; count++) {
    int *received = mb->get<int>();
    value_got = *received;
    delete received;
  }
  xbt_assert(value_got == 3);

  XBT_INFO("OK");
}

static void client(int id)
{
  auto* payload = new int(id);
  XBT_INFO("Sending %d", id);
  sg4::Mailbox::by_name("mymailbox")->put(payload, 10000);
  XBT_INFO("Sent!");
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  std::string platform_file = "small_platform.xml";
  if (argc > 1)
    platform_file = argv[1];
  e.load_platform(platform_file);

  sg4::Actor::create("server", sg4::Host::by_name("Tremblay"), server);
  sg4::Actor::create("client", sg4::Host::by_name("Jupiter"),  client, 1);
  sg4::Actor::create("client", sg4::Host::by_name("Bourassa"), client, 2);
  sg4::Actor::create("client", sg4::Host::by_name("Ginette"),  client, 3);

  e.run();
  return 0;
}
