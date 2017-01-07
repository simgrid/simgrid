/* Copyright (c) 2007, 2009-2016. The SimGrid Team. All rights reserved.    */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_process_kill, "Messages specific for this msg example");

static int victim(int argc, char *argv[])
{
  XBT_INFO("Hello!");
  XBT_INFO("Suspending myself");
  MSG_process_suspend(MSG_process_self()); /* - First suspend itself */
  XBT_INFO("OK, OK. Let's work");          /* - Then is resumed and start to execute a task */
  MSG_task_execute(MSG_task_create("work", 1e9, 0, NULL));
  XBT_INFO("Bye!");  /* - But will never reach the end of it */
  return 0;
}

static int killer(int argc, char *argv[])
{
  XBT_INFO("Hello!");         /* - First start a victim process */
  msg_process_t poor_victim = MSG_process_create("victim", victim, NULL, MSG_host_by_name("Fafard"));
  MSG_process_sleep(10.0);

  XBT_INFO("Resume process"); /* - Resume it from its suspended state */
  MSG_process_resume(poor_victim);

  XBT_INFO("Kill process");   /* - and then kill it */
  MSG_process_kill(poor_victim);

  XBT_INFO("OK, goodbye now.");
  return 0;
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  xbt_assert(argc == 2, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);   /* - Load the platform description */
  /* - Create and deploy killer process, that will create the victim process  */
  MSG_process_create("killer", killer, NULL, MSG_host_by_name("Tremblay"));

  msg_error_t res = MSG_main(); /* - Run the simulation */

  XBT_INFO("Simulation time %g", MSG_get_clock());
  return res != MSG_OK;
}
