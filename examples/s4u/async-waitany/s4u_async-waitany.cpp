/* Copyright (c) 2010-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u.hpp"
#include "xbt/str.h"            

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_async_waitany, "Messages specific for this msg example");

static int sender(int argc, char *argv[])
{
  xbt_assert(argc==6, "This function expects 5 parameters from the XML deployment file");
  long number_of_tasks = xbt_str_parse_int(argv[1], "Invalid amount of tasks: %s");
  double task_comp_size = xbt_str_parse_double(argv[2], "Invalid computational size: %s");
  double task_comm_size = xbt_str_parse_double(argv[3], "Invalid communication size: %s");
  long receivers_count = xbt_str_parse_int(argv[4], "Invalid amount of receivers: %s");
  int diff_com = xbt_str_parse_int(argv[5], "Invalid value for diff_comm: %s");

  std::vector<simgrid::s4u::CommPtr> comms;
  simgrid::s4u::MailboxPtr mbox;
  /* First pack the communications in the dynar */
  for (int i = 0; i < number_of_tasks; i++) {
    double coef = (diff_com == 0) ? 1 : (i + 1);

    char mailbox[80];
    char taskname[80];
    snprintf(mailbox,79, "receiver-%ld", (i % receivers_count));
    snprintf(taskname,79, "Task_%d", i);

    simgrid::s4u::CommPtr comm = mbox->put_async((void*)taskname, 42.0);
    comms.push_back(comm);
    XBT_INFO("Send to receiver-%ld %s comm_size %f", i % receivers_count, taskname, task_comm_size / coef);
  }

  /* Here we are waiting for the completion of all communications */
  while (!comms.empty()) {
    simgrid::s4u::CommPtr  comm;
    comms.pop_back(comm);
    comm->wait();
    delete comm;
  }
  comms.clear();

  /* Here we are waiting for the completion of all tasks */
  for (int i = 0; i < receivers_count; i++) {
    std::sring received = nullptr;
    simgrid::s4u::CommPtr comm = mbox->get_async(&received);
    comm->wait();
    delete comm;
  }

  XBT_INFO("Goodbye now!");
  return 0;
}

static int receiver(int argc, char *argv[])
{
  xbt_assert(argc==3, "This function expects 2 parameters from the XML deployment file");
  int id = xbt_str_parse_int(argv[1], "ID should be numerical, not %s");
  int task_amount = xbt_str_parse_int(argv[2], "Invalid amount of tasks: %s");
  void *received; 
  std::vector<simgrid::s4u::CommPtr> * comms;
  simgrid::s4u::CommPtr comm;
  simgrid::s4u::MailboxPtr mbox;
  
  simgrid::s4u::Actor::sleep_for(10.0);
  for (int i = 0; i < task_amount; i++) {
    XBT_INFO("Wait to receive task %d", i);
    received[i] = NULL;
    comm = mbox->get_async(&received[i]);
    comms.push_back(comm);
  }

  /* Here we are waiting for the receiving of all communications */
  while (!comms.empty()) {
    simgrid::s4u::CommPtr comm;
    // MSG_comm_waitany returns the rank of the comm that just ended. Remove it.
    comms.pop_back(&comm);
    simgrid::s4u::this_actor::execute(comm);
    delete comm;
  }
  comms.clear();

  /* Here we tell to sender that all tasks are done */
  simgrid::s4u::Mailbox::byName("finalize")->put(nullptr, 1);
  XBT_INFO("I'm done. See you!");
  return 0;
}

int main(int argc, char *argv[])
{
  simgrid::s4u::Engine *e = new simgrid::s4u::Engine(&argc,argv);  

  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
                  "\tExample: %s msg_platform.xml msg_deployment.xml\n", argv[0], argv[0]);
 
  e->registerFunction<Sender>("sender");   
  e->registerFunction<Receiver>("receiver");  
  e->loadDeployment(argv[2]); // And then, you load the deployment file

  e->run();

  delete e;
  return 0;
}
