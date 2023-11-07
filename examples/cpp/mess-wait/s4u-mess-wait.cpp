/* Copyright (c) 2023. The SimGrid Team. All rights reserved.          */

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
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_mess_wait, "Messages specific for this s4u example");

static void sender(int messages_count)
{
  sg4::MessageQueue* mqueue = sg4::MessageQueue::by_name("control");

  sg4::this_actor::sleep_for(0.5);

  for (int i = 0; i < messages_count; i++) {
    std::string msg_content = "Message " + std::to_string(i);
    // Copy the data we send: the 'msg_content' variable is not a stable storage location.
    // It will be destroyed when this actor leaves the loop, ie before the receiver gets the data
    auto* payload = new std::string(msg_content);

    /* Create a control message and put it in the message queue */
    sg4::MessPtr mess = mqueue->put_async(payload);
    XBT_INFO("Send '%s' to '%s'", msg_content.c_str(), mqueue->get_cname());
    mess->wait();
  }

  /* Send message to let the receiver know that it should stop */
  XBT_INFO("Send 'finalize' to 'receiver'");
  mqueue->put(new std::string("finalize"));
}

/* Receiver actor expects 1 argument: its ID */
static void receiver()
{
  sg4::MessageQueue* mqueue = sg4::MessageQueue::by_name("control");

  sg4::this_actor::sleep_for(1);

  XBT_INFO("Wait for my first message");
  for (bool cont = true; cont;) {
    std::string* received;
    sg4::MessPtr mess = mqueue->get_async<std::string>(&received);

    sg4::this_actor::sleep_for(0.1);
    mess->wait();

    XBT_INFO("I got a '%s'.", received->c_str());
    if (*received == "finalize")
      cont = false; // If it's a finalize message, we're done.
    delete received;
  }
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  e.load_platform(argv[1]);

  sg4::Actor::create("sender", e.host_by_name("Tremblay"), sender, 3);
  sg4::Actor::create("receiver", e.host_by_name("Fafard"), receiver);

  e.run();

  return 0;
}
