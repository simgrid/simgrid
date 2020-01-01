/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

/* Main function of the process I want to start manually */
static int process_function(XBT_ATTRIB_UNUSED int argc, XBT_ATTRIB_UNUSED char* argv[])
{
  msg_task_t task = MSG_task_create("task", 100, 0, NULL);
  MSG_task_execute (task);
  MSG_task_destroy (task);
  return 0;
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);
  MSG_process_create("simple_func", process_function, NULL, MSG_get_host_by_name("Tremblay"));
  MSG_process_create("simple_func", process_function, NULL, MSG_get_host_by_name("Fafard"));

  MSG_main();
  return 0;
}
