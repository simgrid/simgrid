/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

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

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_comm_wait, "Messages specific for this s4u example");

static void sender(int messages_count, size_t payload_size)
{
  double sleep_start_time = 5.0;
  double sleep_test_time  = 0;

  sg4::Mailbox* mbox = sg4::Mailbox::by_name("receiver");

  XBT_INFO("sleep_start_time : %f , sleep_test_time : %f", sleep_start_time, sleep_test_time);
  sg4::this_actor::sleep_for(sleep_start_time);

  for (int i = 0; i < messages_count; i++) {
    std::string msg_content = std::string("Message ") + std::to_string(i);
    // Copy the data we send: the 'msg_content' variable is not a stable storage location.
    // It will be destroyed when this actor leaves the loop, ie before the receiver gets the data
    auto* payload = new std::string(msg_content);

    /* Create a communication representing the ongoing communication and then */
    sg4::CommPtr comm = mbox->put_async(payload, payload_size);
    XBT_INFO("Send '%s' to '%s'", msg_content.c_str(), mbox->get_cname());

    if (sleep_test_time > 0) {   /* - "test_time" is set to 0, wait */
      while (not comm->test()) { /* - Call test() every "sleep_test_time" otherwise */
        sg4::this_actor::sleep_for(sleep_test_time);
      }
    } else {
      comm->wait();
    }
  }

  /* Send message to let the receiver know that it should stop */
  XBT_INFO("Send 'finalize' to 'receiver'");
  mbox->put(new std::string("finalize"), 0);
}

/* Receiver actor expects 1 argument: its ID */
static void receiver()
{
  double sleep_start_time = 1.0;
  double sleep_test_time  = 0.1;

  sg4::Mailbox* mbox = sg4::Mailbox::by_name("receiver");

  XBT_INFO("sleep_start_time : %f , sleep_test_time : %f", sleep_start_time, sleep_test_time);
  sg4::this_actor::sleep_for(sleep_start_time);

  XBT_INFO("Wait for my first message");
  for (bool cont = true; cont;) {
    std::string* received;
    sg4::CommPtr comm = mbox->get_async<std::string>(&received);

    if (sleep_test_time > 0) {   /* - "test_time" is set to 0, wait */
      while (not comm->test()) { /* - Call test() every "sleep_test_time" otherwise */
        sg4::this_actor::sleep_for(sleep_test_time);
      }
    } else {
      comm->wait();
    }

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

  sg4::Actor::create("sender", e.host_by_name("Tremblay"), sender, 3, 482117300);
  sg4::Actor::create("receiver", e.host_by_name("Ruby"), receiver);

  e.run();

  return 0;
}
