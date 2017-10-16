/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to use simgrid::s4u::this_actor::wait_any() to wait for the first occurring event.
 *
 * As for the other asynchronous examples, the sender initiate all the messages it wants to send and
 * pack the resulting simgrid::s4u::CommPtr objects in a vector. All messages thus occurs concurrently.
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

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_async_waitall, "Messages specific for this msg example");

class Sender {
  long messages_count;  /* - number of tasks */
  long receivers_count; /* - number of receivers */
  double msg_size;      /* - communication cost in bytes */

public:
  explicit Sender(std::vector<std::string> args)
  {
    xbt_assert(args.size() == 4, "Expecting 3 parameters from the XML deployment file but got %zu", args.size());
    messages_count  = std::stol(args[1]);
    msg_size        = std::stod(args[2]);
    receivers_count = std::stol(args[3]);
  }
  void operator()()
  {
    std::vector<simgrid::s4u::CommPtr> pending_comms;

    /* Start dispatching all messages to receivers, in a round robin fashion */
    for (int i = 0; i < messages_count; i++) {

      std::string mboxName          = std::string("receiver-") + std::to_string(i % receivers_count);
      simgrid::s4u::MailboxPtr mbox = simgrid::s4u::Mailbox::byName(mboxName);
      std::string msgName           = std::string("Message ") + std::to_string(i);
      std::string* payload          = new std::string(msgName); // copy the data we send:
                                                                // 'msgName' is not a stable storage location
      XBT_INFO("Send '%s' to '%s'", msgName.c_str(), mboxName.c_str());
      /* Create a communication representing the ongoing communication */
      simgrid::s4u::CommPtr comm = mbox->put_async(payload, msg_size);
      /* Add this comm to the vector of all known comms */
      pending_comms.push_back(comm);
    }

    /* Start sending messages to let the workers know that they should stop */
    for (int i = 0; i < receivers_count; i++) {
      std::string mboxName          = std::string("receiver-") + std::to_string(i % receivers_count);
      simgrid::s4u::MailboxPtr mbox = simgrid::s4u::Mailbox::byName(mboxName);
      std::string* payload          = new std::string("finalize"); // Make a copy of the data we will send

      simgrid::s4u::CommPtr comm = mbox->put_async(payload, 0);
      pending_comms.push_back(comm);
      XBT_INFO("Send 'finalize' to 'receiver-%ld'", i % receivers_count);
    }
    XBT_INFO("Done dispatching all messages");

    /* Now that all message exchanges were initiated, wait for their completion, in order of termination.
     *
     * This loop waits for first terminating message with wait_any() and remove it with erase(), until all comms are
     * terminated
     * Even in this simple example, the pending comms do not terminate in the exact same order of creation.
     */
    while (not pending_comms.empty()) {
      int changed_pos = simgrid::s4u::Comm::wait_any(&pending_comms);
      pending_comms.erase(pending_comms.begin() + changed_pos);
      if (changed_pos != 0)
        XBT_INFO("Remove the %dth pending comm: it terminated earlier than another comm that was initiated first.",
                 changed_pos);
    }

    XBT_INFO("Goodbye now!");
  }
};

/* Receiver actor expects 1 argument: its ID */
class Receiver {
  simgrid::s4u::MailboxPtr mbox;

public:
  explicit Receiver(std::vector<std::string> args)
  {
    xbt_assert(args.size() == 2, "Expecting one parameter from the XML deployment file but got %zu", args.size());
    std::string mboxName = std::string("receiver-") + args[1];
    mbox                 = simgrid::s4u::Mailbox::byName(mboxName);
  }
  void operator()()
  {
    XBT_INFO("Wait for my first message");
    for (bool cont = true; cont;) {
      std::string* received = static_cast<std::string*>(mbox->get());
      XBT_INFO("I got a '%s'.", received->c_str());
      cont = (*received != "finalize"); // If it's a finalize message, we're done
      // Receiving the message was all we were supposed to do
      delete received;
    }
  }
};

int main(int argc, char *argv[])
{
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n", argv[0]);

  simgrid::s4u::Engine e(&argc, argv);
  e.registerFunction<Sender>("sender");
  e.registerFunction<Receiver>("receiver");

  e.loadPlatform(argv[1]);
  e.loadDeployment(argv[2]);
  e.run();

  return 0;
}
