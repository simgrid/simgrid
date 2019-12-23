/* Copyright (c) 2010-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/******************** Non-deterministic message ordering  *********************/
/* Server assumes a fixed order in the reception of messages from its clients */
/* which is incorrect because the message ordering is non-deterministic       */
/******************************************************************************/

#include <simgrid/modelchecker.h>
#include <simgrid/s4u.hpp>

XBT_LOG_NEW_DEFAULT_CATEGORY(mc_assert_example, "Logging channel used in this example");

static int server(int worker_amount)
{
  int value_got             = -1;
  simgrid::s4u::Mailbox* mb = simgrid::s4u::Mailbox::by_name("server");
  for (int count = 0; count < worker_amount; count++) {
    const int* msg = static_cast<int*>(mb->get());
    value_got = *msg;
    delete msg;
  }
  /*
   * We assert here that the last message we got (which overwrite any previously received message) is the one from the
   * last worker This will obviously fail when the messages are received out of order.
   */
  MC_assert(value_got == 2);

  XBT_INFO("OK");
  return 0;
}

static int client(int rank)
{
  /* I just send my rank onto the mailbox. It must be passed as a stable memory block (thus the new) so that that
   * memory survives even after the end of the client */

  simgrid::s4u::Mailbox* mailbox = simgrid::s4u::Mailbox::by_name("server");
  mailbox->put(new int(rank), 1 /* communication cost is not really relevant in MC mode */);

  XBT_INFO("Sent!");
  return 0;
}

int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform_file\n", argv[0]);

  e.load_platform(argv[1]);
  auto hosts = e.get_all_hosts();
  xbt_assert(hosts.size() >= 3, "This example requires at least 3 hosts");

  simgrid::s4u::Actor::create("server", hosts[0], &server, 2);
  simgrid::s4u::Actor::create("client1", hosts[1], &client, 1);
  simgrid::s4u::Actor::create("client2", hosts[2], &client, 2);

  e.run();
  return 0;
}
