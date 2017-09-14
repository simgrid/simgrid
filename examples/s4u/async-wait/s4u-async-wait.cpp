/* Copyright (c) 2010-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

 #include "simgrid/s4u.hpp"
 #include "xbt/str.h"
 #include <cstdlib>
 #include <iostream>

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_async_wait, "Messages specific for this s4u example");

/* Main function of the Sender process */
class sender {
  long messages_count;
  long receivers_count;
  double sleep_start_time; /* - start time */
  double sleep_test_time;         /* - test time */
  double msg_size;
  simgrid::s4u::MailboxPtr mbox;
  
public:
  explicit sender(std::vector<std::string> args)
{
  xbt_assert(args.size() == 7, "The sender function expects 6 arguments from the XML deployment file");
  messages_count = std::stol(args[1]);        /* - number of tasks */
  msg_size = std::stod(args[2]); /* - computational cost */
  double task_comm_size = std::stod(args[3]); /* - communication cost */
  receivers_count = std::stol(args[4]);    /* - number of receivers */
  double sleep_start_time = std::stod(args[5]); /* - start time */
  double sleep_test_time = std::stod(args[6]);         /* - test time */
  XBT_INFO("sleep_start_time : %f , sleep_test_time : %f", sleep_start_time, sleep_test_time);
}
void operator()()
{
  for (int i = 0; i < messages_count; i++) {
    char mailbox[80];
    char taskname[80];
    
    std::string mbox_name = std::string("receiver-") + std::to_string(i % receivers_count);
    mbox = simgrid::s4u::Mailbox::byName(mbox_name);
    snprintf(mailbox,79, "receiver-%ld", i % receivers_count);
    snprintf(taskname,79, "Task_%d", i);
    
    /* Create a communication */
    simgrid::s4u::CommPtr comm = mbox->put_async((void*)mailbox, msg_size);
    XBT_INFO("Send to receiver-%ld Task_%d", i % receivers_count, i);
  }
  
  for (int i = 0; i < receivers_count; i++) {
    char mailbox[80];
    char* payload   = xbt_strdup("finalize"); 
    snprintf(mailbox, 79, "receiver-%d", i);
    simgrid::s4u::CommPtr comm = mbox->put_async((void*)payload, 0);
    XBT_INFO("Send to receiver-%d finalize", i);
  }

  XBT_INFO("Goodbye now!");

}
};

/* Receiver process expects 3 arguments: */
class receiver {
  int id;
  double sleep_test_time;
  double sleep_start_time;
  simgrid::s4u::MailboxPtr mbox;
  
public:
  explicit receiver(std::vector<std::string> args)
  {
    xbt_assert(args.size() == 4, "The relay_runner function does not accept any parameter from the XML deployment file");
  id = std::stoi(args[1]);                                        /* - unique id */
  sleep_start_time = std::stod(args[2]); /* - start time */
  sleep_test_time = std::stod(args[3]);   /* - test time */
  XBT_INFO("sleep_start_time : %f , sleep_test_time : %f", sleep_start_time, sleep_test_time);
  std::string mbox_name = std::string("receiver-") + std::to_string(id);
  mbox = simgrid::s4u::Mailbox::byName(mbox_name);
}

void operator()()
{
  char mailbox[80];
  snprintf(mailbox,79, "receiver-%d", id);
  while (1) {
    char* received = static_cast<char*>(mbox->get());
    XBT_INFO("Wait to receive a task");
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
