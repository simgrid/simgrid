/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to use simgrid::s4u::Mailbox::ready() to check for completed communications.
 *
 * We have a number of peers which send and receive messages in two phases:
 * -> sending phase:   each one of them sends a number of messages to the others followed
 *                     by a single "finalize" message.
 * -> receiving phase: each one of them receives all the available messages that reached
 *                     their corresponding mailbox until it has all the needed "finalize"
 *                     messages to know that no more work needs to be done.
 *
 * To avoid doing a wait() over the ongoing communications, each peer makes use of the
 * simgrid::s4u::Mailbox::ready() method. If it returns true then a following get() will fetch the
 * message immediately, if not the peer will sleep for a fixed amount of time before checking again.
 *
 */

#include "simgrid/s4u.hpp"
#include <cstdlib>
#include <iostream>
#include <string>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_async_ready, "Messages specific for this s4u example");

static void peer(int argc, char** argv)
{
  xbt_assert(argc == 5, "Expecting 4 parameters from the XML deployment file but got %d", argc);
  int my_id           = std::stoi(argv[1]); /* - my id */
  long messages_count = std::stol(argv[2]); /* - number of message */
  long msg_size       = std::stol(argv[3]); /* - message size in bytes */
  long peers_count    = std::stol(argv[4]); /* - number of peers */

  /* Set myself as the persistent receiver of my mailbox so that messages start flowing to me as soon as they are put
   * into it */
  simgrid::s4u::Mailbox* my_mbox = simgrid::s4u::Mailbox::by_name(std::string("peer-") + std::to_string(my_id));
  my_mbox->set_receiver(simgrid::s4u::Actor::self());

  std::vector<simgrid::s4u::CommPtr> pending_comms;

  /* Start dispatching all messages to peers others that myself */
  for (int i = 0; i < messages_count; i++) {
    for (int peer_id = 0; peer_id < peers_count; peer_id++) {
      if (peer_id != my_id) {
        std::string mboxName        = std::string("peer-") + std::to_string(peer_id);
        simgrid::s4u::Mailbox* mbox = simgrid::s4u::Mailbox::by_name(mboxName);
        std::string msgName =
            std::string("Message ") + std::to_string(i) + std::string(" from peer ") + std::to_string(my_id);
        auto* payload = new std::string(msgName); // copy the data we send:
        // 'msgName' is not a stable storage location
        XBT_INFO("Send '%s' to '%s'", msgName.c_str(), mboxName.c_str());
        /* Create a communication representing the ongoing communication */
        pending_comms.push_back(mbox->put_async(payload, msg_size));
      }
    }
  }

  /* Start sending messages to let peers know that they should stop */
  for (int peer_id = 0; peer_id < peers_count; peer_id++) {
    if (peer_id != my_id) {
      std::string mboxName        = std::string("peer-") + std::to_string(peer_id);
      simgrid::s4u::Mailbox* mbox = simgrid::s4u::Mailbox::by_name(mboxName);
      auto* payload               = new std::string("finalize"); // Make a copy of the data we will send
      pending_comms.push_back(mbox->put_async(payload, msg_size));
      XBT_INFO("Send 'finalize' to 'peer-%d'", peer_id);
    }
  }
  XBT_INFO("Done dispatching all messages");

  /* Retrieve all the messages other peers have been sending to me until I receive all the corresponding "Finalize"
   * messages */
  long pending_finalize_messages = peers_count - 1;
  while (pending_finalize_messages > 0) {
    if (my_mbox->ready()) {
      double start        = simgrid::s4u::Engine::get_clock();
      auto received       = my_mbox->get_unique<std::string>();
      double waiting_time = simgrid::s4u::Engine::get_clock() - start;
      xbt_assert(
          waiting_time == 0,
          "Expecting the waiting time to be 0 because the communication was supposedly ready, but got %f instead",
          waiting_time);
      XBT_INFO("I got a '%s'.", received->c_str());
      if (*received == "finalize") {
        pending_finalize_messages--;
      }
    } else {
      XBT_INFO("Nothing ready to consume yet, I better sleep for a while");
      simgrid::s4u::this_actor::sleep_for(.01);
    }
  }

  XBT_INFO("I'm done, just waiting for my peers to receive the messages before exiting");
  simgrid::s4u::Comm::wait_all(&pending_comms);

  XBT_INFO("Goodbye now!");
}

int main(int argc, char* argv[])
{
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n", argv[0]);

  simgrid::s4u::Engine e(&argc, argv);
  e.register_function("peer", &peer);

  e.load_platform(argv[1]);
  e.load_deployment(argv[2]);

  e.run();

  return 0;
}
