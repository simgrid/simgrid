/* Copyright (c) 2010-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

 #include "simgrid/s4u.hpp"
 #include "xbt/str.h"  
 #include <cstdlib>
 #include <iostream>

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_async_waitall, "Messages specific for this msg example");

class sender {
  long number_of_tasks             = 0; /* - Number of tasks      */
  long receivers_count             = 0; /* - Number of workers    */

public:
  explicit sender(std::vector<std::string> args)
{
  xbt_assert(args.size()== 4, "This function expects 5 parameters from the XML deployment file");
  number_of_tasks = std::stol(args[0]);
  double task_comp_size = std::stod(args[1]);
  double task_comm_size = std::stod(args[2]);
  receivers_count = std::stol(args[3]);

}
void operator()()
{
  simgrid::s4u::MailboxPtr mbox = simgrid::s4u::Mailbox::byName("sender_mailbox");
  simgrid::s4u::CommPtr* comms = new simgrid::s4u::CommPtr[number_of_tasks + receivers_count] ;

  for (int i = 0; i < number_of_tasks; i++) {
    char mailbox[80];
    char taskname[80];
    snprintf(mailbox,79, "receiver-%ld", i % receivers_count);
    snprintf(taskname,79, "Task_%d", i);
    comms[i] = mbox->put_async((void*)taskname, 42);
    XBT_INFO("Send to receiver-%ld Task_%d", i % receivers_count, i);
  }
  for (int i = 0; i < receivers_count; i++) {
    char mailbox[80];
    snprintf(mailbox,79, "receiver-%ld", i % receivers_count);
    comms[i + number_of_tasks] = mbox->put_async((void*)"finalize", 42);
    XBT_INFO("Send to receiver-%ld finalize", i % receivers_count);
  }

  /* Here we are waiting for the completion of all communications */
  for (int i = 0; i < number_of_tasks + receivers_count; i++)
    comms[i]->wait();

  delete [] comms;
  XBT_INFO("Goodbye now!");
}
};

class receiver {
public:
  explicit receiver(std::vector<std::string> args)
{
  xbt_assert(args.size() == 1,"This function expects 1 parameter from the XML deployment file");
  int id = xbt_str_parse_int(args[0].c_str(), "Any process of this example must have a numerical name, not %s");
  char mailbox[80];
  snprintf(mailbox,79, "receiver-%d", id);

  simgrid::s4u::this_actor::sleep_for(10.0);

  XBT_INFO("I'm done. See you!");
}
void operator()()
{
  simgrid::s4u::MailboxPtr mbox = simgrid::s4u::Mailbox::byName("receiver_mailbox");
  while (1) {
    XBT_INFO("Wait to receive a task");
    void *received = NULL;
    simgrid::s4u::CommPtr comm = mbox->get_async(&received);
    comm->wait();
    std::string* receivedStr = static_cast<std::string*>(received);
    if (receivedStr->compare("finalize") == 0) {
      delete receivedStr;
      break;
    }
    double *comp_size = static_cast<double*>(received);
    /*  - Otherwise, process the task */
    simgrid::s4u::this_actor::execute(*comp_size);
  }
  XBT_INFO("I'm done. See you!");
}
};

int main(int argc, char *argv[])
{
  simgrid::s4u::Engine *e = new simgrid::s4u::Engine(&argc,argv); 

  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
             "\tExample: %s msg_platform.xml msg_deployment.xml\n", argv[0], argv[0]);

  e->registerFunction<sender>("sender");   
  e->registerFunction<receiver>("receiver"); 

  e->loadDeployment(argv[2]); 
  e->run();

  return 0;
}
