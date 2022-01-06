/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to suspend and resume an asynchronous communication. */

#include "simgrid/s4u.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_comm_wait, "Messages specific for this s4u example");

static void sender()
{
  sg4::Mailbox* mbox = sg4::Mailbox::by_name("receiver");

  // Copy the data we send: the 'msg_content' variable is not a stable storage location.
  // It will be destroyed when this actor leaves the loop, ie before the receiver gets the data
  auto* payload = new std::string("Sent message");

  /* Create a communication representing the ongoing communication and then */
  sg4::CommPtr comm = mbox->put_init(payload, 13194230);
  XBT_INFO("Suspend the communication before it starts (remaining: %.0f bytes) and wait a second.",
           comm->get_remaining());
  sg4::this_actor::sleep_for(1);
  XBT_INFO("Now, start the communication (remaining: %.0f bytes) and wait another second.", comm->get_remaining());
  comm->start();
  sg4::this_actor::sleep_for(1);

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

static void receiver()
{
  sg4::Mailbox* mbox = sg4::Mailbox::by_name("receiver");
  XBT_INFO("Wait for the message.");
  auto received = mbox->get_unique<std::string>();

  XBT_INFO("I got '%s'.", received->c_str());
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  e.load_platform(argv[1]);

  sg4::Actor::create("sender", e.host_by_name("Tremblay"), sender);
  sg4::Actor::create("receiver", e.host_by_name("Jupiter"), receiver);

  e.run();

  return 0;
}
