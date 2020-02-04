/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to use simgrid::s4u::this_actor::wait() to wait for a given communication.
 *
 * As for the other asynchronous examples, the sender initiate all the messages it wants to send and
 * pack the resulting simgrid::s4u::CommPtr objects in a vector. All messages thus occurs concurrently.
 *
 * The sender then loops until there is no ongoing communication.
 */

#include "simgrid/s4u.hpp"
#include <cstdlib>
#include <iostream>
#include <string>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_async_wait, "Messages specific for this s4u example");

static int sender(int argc, char** argv)
{
  xbt_assert(argc == 4, "Expecting 3 parameters from the XML deployment file but got %d", argc);
  long messages_count  = std::stol(argv[1]); /* - number of tasks */
  double msg_size      = std::stol(argv[2]); /* - communication cost in bytes */
  long receivers_count = std::stod(argv[3]); /* - number of receivers */

  /* Vector in which we store all ongoing communications */
  std::vector<simgrid::s4u::CommPtr> pending_comms;

  /* Make a vector of the mailboxes to use */
  std::vector<simgrid::s4u::Mailbox*> mboxes;
  for (int i = 0; i < receivers_count; i++)
    mboxes.push_back(simgrid::s4u::Mailbox::by_name(std::string("receiver-") + std::to_string(i)));

  /* Start dispatching all messages to receivers, in a round robin fashion */
  for (int i = 0; i < messages_count; i++) {
    std::string msg_content = std::string("Message ") + std::to_string(i);
    // Copy the data we send: the 'msg_content' variable is not a stable storage location.
    // It will be destroyed when this actor leaves the loop, ie before the receiver gets the data
    std::string* payload = new std::string(msg_content);

    XBT_INFO("Send '%s' to '%s'", msg_content.c_str(), mboxes[i % receivers_count]->get_cname());

    /* Create a communication representing the ongoing communication, and store it in pending_comms */
    simgrid::s4u::CommPtr comm = mboxes[i % receivers_count]->put_async(payload, msg_size);
    pending_comms.push_back(comm);
  }

  /* Start sending messages to let the workers know that they should stop */
  for (int i = 0; i < receivers_count; i++) {
    XBT_INFO("Send 'finalize' to 'receiver-%d'", i);
    simgrid::s4u::CommPtr comm = mboxes[i]->put_async(new std::string("finalize"), 0);
    pending_comms.push_back(comm);
  }
  XBT_INFO("Done dispatching all messages");

  /* Now that all message exchanges were initiated, wait for their completion, in order of creation. */
  while (not pending_comms.empty()) {
    simgrid::s4u::CommPtr comm = pending_comms.back();
    comm->wait();
    pending_comms.pop_back(); // remove it from the list
  }

  XBT_INFO("Goodbye now!");
  return 0;
}

/* Receiver actor expects 1 argument: its ID */
static int receiver(int argc, char** argv)
{
  xbt_assert(argc == 2, "Expecting one parameter from the XML deployment file but got %d", argc);
  simgrid::s4u::Mailbox* mbox = simgrid::s4u::Mailbox::by_name(std::string("receiver-") + argv[1]);

  XBT_INFO("Wait for my first message");
  for (bool cont = true; cont;) {
    const std::string* received = static_cast<std::string*>(mbox->get());
    XBT_INFO("I got a '%s'.", received->c_str());
    if (*received == "finalize")
      cont = false; // If it's a finalize message, we're done.
    delete received;
  }
  return 0;
}

int main(int argc, char *argv[])
{
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n", argv[0]);

  simgrid::s4u::Engine e(&argc, argv);
  e.register_function("sender", &sender);
  e.register_function("receiver", &receiver);

  e.load_platform(argv[1]);
  e.load_deployment(argv[2]);
  e.run();

  return 0;
}
