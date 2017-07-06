/* Copyright (c) 2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <iostream>
#include <simgrid/s4u.hpp>
#include <stdlib.h>
#include <vector>

#define NUM_COMMS 1

XBT_LOG_NEW_DEFAULT_CATEGORY(mwe, "Minimum Working Example");

static void receiver()
{
  simgrid::s4u::MailboxPtr mymailbox = simgrid::s4u::Mailbox::byName("receiver_mailbox");

  std::vector<simgrid::s4u::CommPtr> pending_comms;

  XBT_INFO("Placing %d asynchronous recv requests", NUM_COMMS);
  void* data;
  for (int i = 0; i < NUM_COMMS; i++) {
    simgrid::s4u::CommPtr comm = mymailbox->recv_async(&data);
    pending_comms.push_back(comm);
  }

  for (int i = 0; i < NUM_COMMS; i++) {
    XBT_INFO("Sleeping for 3 seconds (for the %dth time)...", i + 1);
    simgrid::s4u::this_actor::sleep_for(3.0);
    XBT_INFO("Calling wait_any() for %zu pending comms", pending_comms.size());
    int changed_pos = simgrid::s4u::Comm::wait_any(&pending_comms);
    XBT_INFO("Counting the number of completed comms...");

    pending_comms.erase(pending_comms.begin() + changed_pos);
  }
}

static void sender()
{
  simgrid::s4u::MailboxPtr theirmailbox = simgrid::s4u::Mailbox::byName("receiver_mailbox");

  void* data = (void*)"data";

  for (int i = 0; i < NUM_COMMS; i++) {
    XBT_INFO("Sending a message to the receiver");
    theirmailbox->send(&data, 4);
    XBT_INFO("Sleeping for 1000 seconds");
    simgrid::s4u::this_actor::sleep_for(1000.0);
  }
}

int main(int argc, char** argv)
{

  simgrid::s4u::Engine* engine = new simgrid::s4u::Engine(&argc, argv);

  xbt_assert(argc >= 2, "Usage: %s <xml platform file>", argv[0]);

  engine->loadPlatform(argv[1]);
  simgrid::s4u::Host** hosts = sg_host_list();
  simgrid::s4u::Actor::createActor("Receiver", hosts[0], receiver);
  simgrid::s4u::Actor::createActor("Sender", hosts[1], sender);
  xbt_free(hosts);

  engine->run();

  delete engine;
  return 0;
}
