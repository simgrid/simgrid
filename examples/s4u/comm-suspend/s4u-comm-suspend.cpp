/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to suspend and resume an asynchronous communication. */

#include "simgrid/s4u.hpp"
#include <cstdlib>
#include <iostream>
#include <string>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_comm_wait, "Messages specific for this s4u example");

static void sender(int argc, char**)
{
  xbt_assert(argc == 1, "Expecting no parameter from the XML deployment file but got %d", argc - 1);

  simgrid::s4u::Mailbox* mbox = simgrid::s4u::Mailbox::by_name("receiver");

  // Copy the data we send: the 'msg_content' variable is not a stable storage location.
  // It will be destroyed when this actor leaves the loop, ie before the receiver gets the data
  auto* payload = new std::string("Sent message");

  /* Create a communication representing the ongoing communication and then */
  simgrid::s4u::CommPtr comm = mbox->put_init(payload, 13194230);
  XBT_INFO("Suspend the communication before it starts (remaining: %.0f bytes) and wait a second.",
           comm->get_remaining());
  simgrid::s4u::this_actor::sleep_for(1);
  XBT_INFO("Now, start the communication (remaining: %.0f bytes) and wait another second.", comm->get_remaining());
  comm->start();
  simgrid::s4u::this_actor::sleep_for(1);

  XBT_INFO("There is still %.0f bytes to transfer in this communication. Suspend it for one second.",
           comm->get_remaining());
  comm->suspend();
  XBT_INFO("Now there is %.0f bytes to transfer. Resume it and wait for its completion.", comm->get_remaining());
  comm->resume();
  comm->wait();
  XBT_INFO("There is %f bytes to transfer after the communication completion.", comm->get_remaining());
  XBT_INFO("Suspending a completed activity is a no-op.");
  comm->suspend();
}

static void receiver(int, char**)
{
  simgrid::s4u::Mailbox* mbox = simgrid::s4u::Mailbox::by_name("receiver");

  XBT_INFO("Wait for the message.");
  void* payload = mbox->get();

  const auto* received = static_cast<std::string*>(payload);
  XBT_INFO("I got '%s'.", received->c_str());

  delete received;
}

int main(int argc, char* argv[])
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
