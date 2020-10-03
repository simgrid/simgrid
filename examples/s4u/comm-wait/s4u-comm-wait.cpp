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

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_comm_wait, "Messages specific for this s4u example");

static void sender(int argc, char** argv)
{
  xbt_assert(argc == 3, "Expecting 2 parameters from the XML deployment file but got %d", argc);
  long messages_count     = std::stol(argv[1]); /* - number of messages */
  long msg_size           = std::stol(argv[2]); /* - message size in bytes */
  double sleep_start_time = 5.0;
  double sleep_test_time  = 0;

  simgrid::s4u::Mailbox* mbox = simgrid::s4u::Mailbox::by_name("receiver");

  XBT_INFO("sleep_start_time : %f , sleep_test_time : %f", sleep_start_time, sleep_test_time);
  simgrid::s4u::this_actor::sleep_for(sleep_start_time);

  for (int i = 0; i < messages_count; i++) {
    std::string msg_content = std::string("Message ") + std::to_string(i);
    // Copy the data we send: the 'msg_content' variable is not a stable storage location.
    // It will be destroyed when this actor leaves the loop, ie before the receiver gets the data
    auto* payload = new std::string(msg_content);

    /* Create a communication representing the ongoing communication and then */
    simgrid::s4u::CommPtr comm = mbox->put_async(payload, msg_size);
    XBT_INFO("Send '%s' to '%s'", msg_content.c_str(), mbox->get_cname());

    if (sleep_test_time > 0) {   /* - "test_time" is set to 0, wait */
      while (not comm->test()) { /* - Call test() every "sleep_test_time" otherwise */
        simgrid::s4u::this_actor::sleep_for(sleep_test_time);
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
static void receiver(int, char**)
{
  double sleep_start_time = 1.0;
  double sleep_test_time  = 0.1;

  simgrid::s4u::Mailbox* mbox = simgrid::s4u::Mailbox::by_name("receiver");

  XBT_INFO("sleep_start_time : %f , sleep_test_time : %f", sleep_start_time, sleep_test_time);
  simgrid::s4u::this_actor::sleep_for(sleep_start_time);

  XBT_INFO("Wait for my first message");
  for (bool cont = true; cont;) {
    void* payload;
    simgrid::s4u::CommPtr comm = mbox->get_async(&payload);

    if (sleep_test_time > 0) {   /* - "test_time" is set to 0, wait */
      while (not comm->test()) { /* - Call test() every "sleep_test_time" otherwise */
        simgrid::s4u::this_actor::sleep_for(sleep_test_time);
      }
    } else {
      comm->wait();
    }

    const auto* received = static_cast<std::string*>(payload);
    XBT_INFO("I got a '%s'.", received->c_str());
    if (*received == "finalize")
      cont = false; // If it's a finalize message, we're done.
    delete received;
  }
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
