/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

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
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_async_ready, "Messages specific for this s4u example");

static void peer(int my_id, int messages_count, size_t payload_size, int peers_count)
{
  /* Set myself as the persistent receiver of my mailbox so that messages start flowing to me as soon as they are put
   * into it */
  sg4::Mailbox* my_mbox = sg4::Mailbox::by_name(std::string("peer-") + std::to_string(my_id));
  my_mbox->set_receiver(sg4::Actor::self());

  std::vector<sg4::CommPtr> pending_comms;

  /* Start dispatching all messages to peers others that myself */
  for (int i = 0; i < messages_count; i++) {
    for (int peer_id = 0; peer_id < peers_count; peer_id++) {
      if (peer_id != my_id) {
        sg4::Mailbox* mbox  = sg4::Mailbox::by_name(std::string("peer-") + std::to_string(peer_id));
        std::string message = std::string("Message ") + std::to_string(i) + " from peer " + std::to_string(my_id);
        auto* payload       = new std::string(message); // copy the data we send:
        // 'msgName' is not a stable storage location
        XBT_INFO("Send '%s' to '%s'", message.c_str(), mbox->get_cname());
        /* Create a communication representing the ongoing communication */
        pending_comms.push_back(mbox->put_async(payload, payload_size));
      }
    }
  }

  /* Start sending messages to let peers know that they should stop */
  for (int peer_id = 0; peer_id < peers_count; peer_id++) {
    if (peer_id != my_id) {
      sg4::Mailbox* mbox = sg4::Mailbox::by_name(std::string("peer-") + std::to_string(peer_id));
      auto* payload      = new std::string("finalize"); // Make a copy of the data we will send
      pending_comms.push_back(mbox->put_async(payload, payload_size));
      XBT_INFO("Send 'finalize' to 'peer-%d'", peer_id);
    }
  }
  XBT_INFO("Done dispatching all messages");

  /* Retrieve all the messages other peers have been sending to me until I receive all the corresponding "Finalize"
   * messages */
  long pending_finalize_messages = peers_count - 1;
  while (pending_finalize_messages > 0) {
    if (my_mbox->ready()) {
      double start        = sg4::Engine::get_clock();
      auto received       = my_mbox->get_unique<std::string>();
      double waiting_time = sg4::Engine::get_clock() - start;
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
      sg4::this_actor::sleep_for(.01);
    }
  }

  XBT_INFO("I'm done, just waiting for my peers to receive the messages before exiting");
  sg4::Comm::wait_all(pending_comms);

  XBT_INFO("Goodbye now!");
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  sg4::Actor::create("peer", e.host_by_name("Tremblay"), peer, 0, 2, 5e7, 3);
  sg4::Actor::create("peer", e.host_by_name("Ruby"), peer, 1, 6, 2.5e5, 3);
  sg4::Actor::create("peer", e.host_by_name("Perl"), peer, 2, 0, 5e7, 3);

  e.run();

  return 0;
}
