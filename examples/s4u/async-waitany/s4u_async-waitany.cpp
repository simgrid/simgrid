/* Copyright (c) 2010-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "xbt/str.h"  
#include <cstdlib>
#include <iostream>
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_async_waitany, "Messages specific for this msg example");

class sender {
public:
  explicit sender(std::vector<std::string> args)
{
  xbt_assert(args.size()== 5, "This function expects 5 parameters from the XML deployment file");
  long number_of_tasks = xbt_str_parse_int(args[0].c_str(), "Invalid amount of tasks: %s");
  double task_comp_size = xbt_str_parse_double(args[1].c_str(), "Invalid computational size: %s");
  double task_comm_size = xbt_str_parse_double(args[2].c_str(), "Invalid communication size: %s");
  long receivers_count = xbt_str_parse_int(args[3].c_str(), "Invalid amount of receivers: %s");
  int diff_com = xbt_str_parse_int(args[4].c_str(), "Invalid value for diff_comm: %s");

  std::vector<simgrid::s4u::CommPtr> comms;
  simgrid::s4u::CommPtr comm;
  simgrid::s4u::MailboxPtr mbox = simgrid::s4u::Mailbox::byName("receiver_mailbox");
  /* First pack the communications in the dynar */
  for (int i = 0; i < number_of_tasks; i++) {
    double coef = (diff_com == 0) ? 1 : (i + 1);

    char mailbox[80];
    char taskname[80];
    snprintf(mailbox,79, "receiver-%ld", (i % receivers_count));
    snprintf(taskname,79, "Task_%d", i);

    comm = mbox->put_async((void*)taskname, 42.0);
    comms.push_back(comm);
    XBT_INFO("Send to receiver-%ld %s comm_size %f", i % receivers_count, taskname, task_comm_size / coef);
  }

  /* Here we are waiting for the completion of all communications */
  while (!comms.empty()) {
    comms.pop_back();
  }
  comms.clear();

  /* Here we are waiting for the completion of all tasks */
  for (int i = 0; i < receivers_count; i++) {
    void* received = nullptr;
    simgrid::s4u::CommPtr comm = mbox->get_async(&received);
    comm->wait();
    comm = nullptr;
  }

  XBT_INFO("Goodbye now!");
}
};

class receiver {
public:
  explicit receiver(std::vector<std::string> args)
{
  xbt_assert(args.size() == 2, "This function expects 2 parameters from the XML deployment file");
  int id = xbt_str_parse_int(args[0].c_str(), "ID should be numerical, not %s");
  int task_amount = xbt_str_parse_int(args[1].c_str(), "Invalid amount of tasks: %s");
  void *received; 
  std::vector<simgrid::s4u::CommPtr> comms;
  simgrid::s4u::CommPtr comm;
  simgrid::s4u::MailboxPtr mbox;
  
  simgrid::s4u::this_actor::sleep_for(10.0);
  for (int i = 0; i < task_amount; i++) {
    XBT_INFO("Wait to receive task %d", i);
    received = NULL;
    comm = mbox->get_async(&received);
    comms.push_back(comm);
  }

  /* Here we are waiting for the receiving of all communications */
  while (!comms.empty()) {
    // returns the rank of the comm that just ended. Remove it.
    comm=comms.back();
    comms.pop_back();
    // simgrid::s4u::this_actor::execute(comm);
  }
  comms.clear();
  comm = nullptr;

  /* Here we tell to sender that all tasks are done */
  simgrid::s4u::Mailbox::byName("finalize")->put(nullptr, 1);
  XBT_INFO("I'm done. See you!");
}

int main(int argc, char *argv[])
{
  simgrid::s4u::Engine *e = new simgrid::s4u::Engine(&argc,argv);  

  e->loadDeployment(argv[2]); 
 
  e->registerFunction<sender>("sender");   
  // e->registerFunction<receiver>("receiver");  

  e->run();

  delete e;
  return 0;
}
