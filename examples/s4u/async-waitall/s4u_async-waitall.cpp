/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to use asynchronous communications.
 *
 * The sender initiate all the messages it wants to send, and then block for their completion.
 * All messages thus occurs concurrently.
 *
 * On the receiver side, the reception is synchronous.
 *
 * TODO: this example is supposed to test the waitall function, but this is not ported to s4u yet.
 *
 */

#include "simgrid/s4u.hpp"
#include "xbt/str.h"
#include <cstdlib>
#include <iostream>

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_async_waitall, "Messages specific for this msg example");

class sender {
  long messages_count;
  long receivers_count;
  double msg_size; /* in bytes */

public:
  explicit sender(std::vector<std::string> args)
{
  xbt_assert(args.size() == 4, "This function expects 4 parameters from the XML deployment file but got %zu",
             args.size());
  messages_count  = std::stol(args[1]);
  msg_size        = std::stod(args[2]);
  receivers_count = std::stol(args[3]);

}
void operator()()
{
  std::vector<simgrid::s4u::CommPtr>* pending_comms = new std::vector<simgrid::s4u::CommPtr>();

  /* Start dispatching all messages to receivers, in a round robin fashion */
  for (int i = 0; i < messages_count; i++) {

    std::string mboxName          = std::string("receiver-") + std::to_string(i % receivers_count);
    simgrid::s4u::MailboxPtr mbox = simgrid::s4u::Mailbox::byName(mboxName);
    std::string msgName           = std::string("Message ") + std::to_string(i);
    char* payload = xbt_strdup(msgName.c_str()); // copy the data we send: 'msgName' is not a stable storage location

    XBT_INFO("Send '%s' to '%s'", msgName.c_str(), mboxName.c_str());
    /* Create a communication representing the ongoing communication */
    simgrid::s4u::CommPtr comm = mbox->put_async((void*)payload, msg_size);
    /* Add this comm to the vector of all known comms */
    pending_comms->push_back(comm);
  }
  /* Start sending messages to let the workers know that they should stop */
  for (int i = 0; i < receivers_count; i++) {
    std::string mbox_name         = std::string("receiver-") + std::to_string(i % receivers_count);
    simgrid::s4u::MailboxPtr mbox = simgrid::s4u::Mailbox::byName(mbox_name);
    char* payload                 = xbt_strdup("finalize"); // Make a copy of the data we will send

    simgrid::s4u::CommPtr comm = mbox->put_async((void*)payload, 0);
    pending_comms->push_back(comm);
    XBT_INFO("Send 'finalize' to 'receiver-%ld'", i % receivers_count);
  }
  XBT_INFO("Done dispatching all messages");

  /* Now that all message exchanges were initiated, this loop waits for the termination of them all */
  for (int i = 0; i < messages_count + receivers_count; i++)
    pending_comms->at(i)->wait();

  XBT_INFO("Goodbye now!");
  delete pending_comms;
}
};

class receiver {
  simgrid::s4u::MailboxPtr mbox;

public:
  explicit receiver(std::vector<std::string> args)
{
  xbt_assert(args.size() == 2, "This function expects 2 parameters from the XML deployment file but got %zu",
             args.size());
  int id = xbt_str_parse_int(args[1].c_str(), "Any process of this example must have a numerical name, not %s");
  std::string mbox_name = std::string("receiver-") + std::to_string(id);
  mbox                  = simgrid::s4u::Mailbox::byName(mbox_name);
}
void operator()()
{
  XBT_INFO("Wait for my first message");
  while (1) {
    char* received = static_cast<char*>(mbox->get());
    XBT_INFO("I got a '%s'.", received);
    if (std::strcmp(received, "finalize") == 0) { /* If it's a finalize message, we're done */
      xbt_free(received);
      break;
    }
    /* Otherwise receiving the message was all we were supposed to do */
    xbt_free(received);
  }
}
};

int main(int argc, char *argv[])
{
  simgrid::s4u::Engine* e = new simgrid::s4u::Engine(&argc, argv);

  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n", argv[0]);

  e->registerFunction<sender>("sender");
  e->registerFunction<receiver>("receiver");

  e->loadPlatform(argv[1]);
  e->loadDeployment(argv[2]); 
  e->run();

  return 0;
}
