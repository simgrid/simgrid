/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

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

static void server()
{
  void* received = nullptr;
  int count      = 0;
  while (count < N) {
    if (received) {
      delete static_cast<int*>(received);
      received = nullptr;
    }
    received = simgrid::s4u::Mailbox::by_name("mymailbox")->get();
    count++;
  }
  int value_got = *(static_cast<int*>(received));
  MC_assert(value_got == 3);

  XBT_INFO("OK");
}

static void client(int id)
{
  int* payload = new int();
  *payload     = id;
  simgrid::s4u::Mailbox::by_name("mymailbox")->put(payload, 10000);

  XBT_INFO("Sent!");
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  e.load_platform(argv[1]);

  simgrid::s4u::Actor::create("server", simgrid::s4u::Host::by_name("HostA"), server);
  simgrid::s4u::Actor::create("client", simgrid::s4u::Host::by_name("HostB"), client, 1);
  simgrid::s4u::Actor::create("client", simgrid::s4u::Host::by_name("HostC"), client, 2);
  simgrid::s4u::Actor::create("client", simgrid::s4u::Host::by_name("HostD"), client, 3);

  e.run();
  return 0;
}
