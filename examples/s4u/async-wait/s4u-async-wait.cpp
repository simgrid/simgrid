/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This example shows how to start many asynchronous communications,
 * and block on them later.
 */

#include "simgrid/s4u.hpp"
#include "xbt/str.h"
#include <cstdlib>
#include <iostream>

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_async_wait, "Messages specific for this s4u example");

/* Main function of the Sender process */
class sender {
  long messages_count;             /* - number of tasks */
  long receivers_count;            /* - number of receivers */
  double msg_size;                 /* - communication cost in bytes */
  simgrid::s4u::MailboxPtr mbox;
  
public:
  explicit sender(std::vector<std::string> args)
{
  xbt_assert(args.size() == 4, "The sender function expects 3 arguments from the XML deployment file");
  messages_count = std::stol(args[1]);
  msg_size        = std::stod(args[2]);
  receivers_count = std::stol(args[3]);
}
void operator()()
{
  std::vector<simgrid::s4u::CommPtr>* pending_comms = new std::vector<simgrid::s4u::CommPtr>();

  /* Start dispatching all messages to receivers, in a round robin fashion */
  for (int i = 0; i < messages_count; i++) {
    
    std::string mbox_name = std::string("receiver-") + std::to_string(i % receivers_count);
    mbox = simgrid::s4u::Mailbox::byName(mbox_name);
    
    /* Create a communication representing the ongoing communication */
    std::string msgName = std::string("Message ") + std::to_string(i);
    char* payload = xbt_strdup(msgName.c_str()); // copy the data we send: 'msgName' is not a stable storage location

    simgrid::s4u::CommPtr comm = mbox->put_async((void*)payload, msg_size);
    XBT_INFO("Send to receiver-%ld Task_%d", i % receivers_count, i);
    pending_comms->push_back(comm);
  }

  /* Now that all comms are in flight, wait for all of them (one after the other) */
  for (int i = 0; i < messages_count; i++) {
    while (not pending_comms->empty()) {
      simgrid::s4u::CommPtr comm = pending_comms->back();
      comm->wait();              // we could provide a timeout as a parameter
      pending_comms->pop_back(); // remove it from the list
    }
  }

  /* Start sending messages to let the workers know that they should stop (in a synchronous way) */
  for (int i = 0; i < receivers_count; i++) {
    char mailbox[80];
    char* payload   = xbt_strdup("finalize"); 
    snprintf(mailbox, 79, "receiver-%d", i);
    mbox->put((void*)payload, 0); // instantaneous message (payload size is 0) sent in a synchronous way (with put)
    XBT_INFO("Send to receiver-%d finalize", i);
  }

  XBT_INFO("Goodbye now!");

}
};

/* Receiver process expects 1 arguments: its ID */
class receiver {
  simgrid::s4u::MailboxPtr mbox;
  
public:
  explicit receiver(std::vector<std::string> args)
  {
    xbt_assert(args.size() == 2, "The receiver function takes a unique parameter from the XML deployment file");
    std::string mbox_name = std::string("receiver-") + args[1];
    mbox                  = simgrid::s4u::Mailbox::byName(mbox_name);
}

void operator()()
{
  while (1) {
    XBT_INFO("Wait to receive a task");
    char* received = static_cast<char*>(mbox->get());
    XBT_INFO("I got a '%s'.", received);
    if (std::strcmp(received, "finalize") == 0) { /* If it's a finalize message, we're done */
      xbt_free(received);
      break;
    }
  }
}
};

int main(int argc, char *argv[])
{
  simgrid::s4u::Engine e(&argc, argv);

  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n", argv[0]);

  e.registerFunction<sender>("sender");
  e.registerFunction<receiver>("receiver");

  e.loadPlatform(argv[1]);
  e.loadDeployment(argv[2]);
  e.run();

  return 0;
}
