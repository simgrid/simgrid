/* Copyright (c) 2010-2016. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_async_waitall, "Messages specific for this msg example");

class sender {
public:
  explicit sender(std::vector<std::string> args)
{
  xbt_assert(args.size()== 4, "This function expects 5 parameters from the XML deployment file");
  long number_of_tasks = xbt_str_parse_int(args[0].c_str(), "Invalid amount of tasks: %s");
  double task_comp_size = xbt_str_parse_double(args[1].c_str(), "Invalid computational size: %s");
  double task_comm_size = xbt_str_parse_double(args[2].c_str(), "Invalid communication size: %s");
  long receivers_count = xbt_str_parse_int(args[3].c_str(), "Invalid amount of receivers: %s");

  simgrid::s4u::CommPtr* comms = new simgrid::s4u::CommPtr[number_of_tasks + receivers_count] ;

  for (int i = 0; i < number_of_tasks; i++) {
    char mailbox[80];
    char taskname[80];
    snprintf(mailbox,79, "receiver-%ld", i % receivers_count);
    snprintf(taskname,79, "Task_%d", i);
    comms[i] = mbox->put_async((void*)mailbox, 42.0);
    XBT_INFO("Send to receiver-%ld Task_%d", i % receivers_count, i);
  }
  for (int i = 0; i < receivers_count; i++) {
    char mailbox[80];
    snprintf(mailbox,79, "receiver-%ld", i % receivers_count);

    comms[i + number_of_tasks] = mbox->put_async((void*)mailbox, 42.0);
    
    XBT_INFO("Send to receiver-%ld finalize", i % receivers_count);
  }

  /* Here we are waiting for the completion of all communications */
  for (int i = 0; i < number_of_tasks + receivers_count; i++)
    comms[i]->wai();

  delete [] comms;
}
void operator()()
{
  XBT_INFO("Goodbye now!");
}
};

class receiver {
public:
  explicit receiver(std::vector<std::string> args)
{
  xbt_assert(args.size() == 1,"This function expects 1 parameter from the XML deployment file");
  int id = xbt_str_parse_int(args[0].c_str(), "Any process of this example must have a numerical name, not %s");
  void *received;
  char mailbox[80];
  snprintf(mailbox,79, "receiver-%d", id);

  simgrid::s4u::this_actor::sleep_for(10.0);
  while (1) {
    XBT_INFO("Wait to receive a task");
    received = NULL;
    comm = mbox->get_async(&received);
    comm->wait();
    if (strcmp(MSG_task_get_name(task), "finalize") == 0) {
      MSG_task_destroy(task);
      break;
    }

    XBT_INFO("Processing \"%s\"", MSG_task_get_name(task));
    MSG_task_execute(task);
    XBT_INFO("\"%s\" done", MSG_task_get_name(task));
    MSG_task_destroy(task);
  }
  XBT_INFO("I'm done. See you!");
}
void operator()()
{
  simgrid::s4u::Mailbox::byName("finalize")->put(nullptr, 1);
  XBT_INFO("I'm done. See you!");
}
};

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
             "\tExample: %s msg_platform.xml msg_deployment.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);
  MSG_function_register("sender", sender);
  MSG_function_register("receiver", receiver);
  MSG_launch_application(argv[2]);

  msg_error_t res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}
