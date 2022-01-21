/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to use simgrid::s4u::this_actor::wait_any() to wait for the first occurring event.
 *
 * As for the other asynchronous examples, the sender initiate all the messages it wants to send and
 * pack the resulting simgrid::s4u::CommPtr objects in a vector. All messages thus occur concurrently.
 *
 * The sender then loops until there is no ongoing communication. Using wait_any() ensures that the sender
 * will notice events as soon as they occur even if it does not follow the order of the container.
 *
 * Here, finalize messages will terminate earlier because their size is 0, so they travel faster than the
 * other messages of this application.  As expected, the trace shows that the finalize of worker 1 is
 * processed before 'Message 5' that is sent to worker 0.
 *
 */

#include "simgrid/s4u.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_comm_waitall, "Messages specific for this s4u example");

static void sender(unsigned int messages_count, unsigned int receivers_count, long msg_size)
{
  if (messages_count == 0 || receivers_count == 0) {
    XBT_WARN("Sender has nothing to do. Bail out!");
    return;
  }
  /* Vector in which we store all ongoing communications */
  std::vector<sg4::CommPtr> pending_comms;

  /* Make a vector of the mailboxes to use */
  std::vector<sg4::Mailbox*> mboxes;
  for (unsigned int i = 0; i < receivers_count; i++)
    mboxes.push_back(sg4::Mailbox::by_name(std::string("receiver-") + std::to_string(i)));

  /* Start dispatching all messages to receivers, in a round robin fashion */
  for (unsigned int i = 0; i < messages_count; i++) {
    std::string msg_content = std::string("Message ") + std::to_string(i);
    // Copy the data we send: the 'msg_content' variable is not a stable storage location.
    // It will be destroyed when this actor leaves the loop, ie before the receiver gets it
    auto* payload = new std::string(msg_content);

    XBT_INFO("Send '%s' to '%s'", msg_content.c_str(), mboxes[i % receivers_count]->get_cname());

    /* Create a communication representing the ongoing communication, and store it in pending_comms */
    sg4::CommPtr comm = mboxes[i % receivers_count]->put_async(payload, msg_size);
    pending_comms.push_back(comm);
  }

  /* Start sending messages to let the workers know that they should stop */
  for (unsigned int i = 0; i < receivers_count; i++) {
    XBT_INFO("Send 'finalize' to 'receiver-%u'", i);
    sg4::CommPtr comm = mboxes[i]->put_async(new std::string("finalize"), 0);
    pending_comms.push_back(comm);
  }
  XBT_INFO("Done dispatching all messages");

  /* Now that all message exchanges were initiated, wait for their completion, in order of termination.
   *
   * This loop waits for first terminating message with wait_any() and remove it with erase(), until all comms are
   * terminated
   * Even in this simple example, the pending comms do not terminate in the exact same order of creation.
   */
  while (not pending_comms.empty()) {
    ssize_t changed_pos = sg4::Comm::wait_any(pending_comms);
    pending_comms.erase(pending_comms.begin() + changed_pos);
    if (changed_pos != 0)
      XBT_INFO("Remove the %zdth pending comm: it terminated earlier than another comm that was initiated first.",
               changed_pos);
  }

  XBT_INFO("Goodbye now!");
}

/* Receiver actor expects 1 argument: its ID */
static void receiver(int id)
{
  sg4::Mailbox* mbox = sg4::Mailbox::by_name(std::string("receiver-") + std::to_string(id));
  XBT_INFO("Wait for my first message");
  for (bool cont = true; cont;) {
    auto received = mbox->get_unique<std::string>();
    XBT_INFO("I got a '%s'.", received->c_str());
    cont = (*received != "finalize"); // If it's a finalize message, we're done
    // Receiving the message was all we were supposed to do
  }
}

int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);

  e.load_platform(argv[1]);

  sg4::Actor::create("sender", e.host_by_name("Tremblay"), sender, 6, 2, 1e6);
  sg4::Actor::create("receiver", e.host_by_name("Fafard"), receiver, 0);
  sg4::Actor::create("receiver", e.host_by_name("Jupiter"), receiver, 1);

  e.run();

  return 0;
}
