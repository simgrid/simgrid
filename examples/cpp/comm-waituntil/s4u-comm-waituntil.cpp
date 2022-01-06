/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to use simgrid::s4u::Activity::wait_until() and
 * simgrid::s4u::Activity::wait_for() on a given communication.
 *
 * It is very similar to the comm-wait example, but the sender initially
 * does some waits that are too short before doing an infinite wait.
 */

#include "simgrid/s4u.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_comm_waituntil, "Messages specific for this s4u example");

static void sender(int messages_count, size_t payload_size)
{
  std::vector<sg4::CommPtr> pending_comms;
  sg4::Mailbox* mbox = sg4::Mailbox::by_name("receiver-0");

  /* Start dispatching all messages to the receiver */
  for (int i = 0; i < messages_count; i++) {
    std::string message = std::string("Message ") + std::to_string(i);
    auto* payload       = new std::string(message); // copy the data we send:

    // 'msgName' is not a stable storage location
    XBT_INFO("Send '%s' to '%s'", message.c_str(), mbox->get_cname());
    /* Create a communication representing the ongoing communication */
    sg4::CommPtr comm = mbox->put_async(payload, payload_size);
    /* Add this comm to the vector of all known comms */
    pending_comms.push_back(comm);
  }

  /* Start the finalize signal to the receiver*/
  auto* payload     = new std::string("finalize"); // Make a copy of the data we will send
  sg4::CommPtr comm = mbox->put_async(payload, 0);
  pending_comms.push_back(comm);
  XBT_INFO("Send 'finalize' to 'receiver-0'");

  XBT_INFO("Done dispatching all messages");

  /* Now that all message exchanges were initiated, wait for their completion, in order of creation. */
  while (not pending_comms.empty()) {
    sg4::CommPtr comm = pending_comms.back();
    comm->wait_for(1);
    pending_comms.pop_back(); // remove it from the list
  }

  XBT_INFO("Goodbye now!");
}

static void receiver()
{
  sg4::Mailbox* mbox = sg4::Mailbox::by_name("receiver-0");

  XBT_INFO("Wait for my first message");
  for (bool cont = true; cont;) {
    auto received = mbox->get_unique<std::string>();
    XBT_INFO("I got a '%s'.", received->c_str());
    if (*received == "finalize")
      cont = false; // If it's a finalize message, we're done.
  }
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  e.load_platform(argv[1]);

  sg4::Actor::create("sender", e.host_by_name("Tremblay"), sender, 3, 5e7);
  sg4::Actor::create("receiver", e.host_by_name("Ruby"), receiver);

  e.run();

  return 0;
}
