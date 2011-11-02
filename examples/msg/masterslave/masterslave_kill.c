/* Copyright (c) 2007, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg/msg.h"            /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "xbt/sysdep.h"         /* calloc */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

/** Lazy guy function. This process suspends itself asap.  */
static int slave(int argc, char *argv[])
{
  XBT_INFO("Hello!");
  XBT_INFO("Suspend process");
  MSG_process_suspend(MSG_process_self());
  MSG_task_execute(MSG_task_create("toto", 1e9, 0, NULL));
  XBT_INFO("Bye!");
  return 0;
}                               /* end_of_lazy_guy */

static int master(int argc, char *argv[])
{
  m_process_t bob = NULL;

  XBT_INFO("Hello!");
  bob = MSG_process_create("slave", slave, NULL, MSG_get_host_by_name("bob"));
  MSG_process_sleep(10.0);

  XBT_INFO("Resume process");
  MSG_process_resume(bob);

  XBT_INFO("Kill process");
  MSG_process_kill(bob);

  XBT_INFO("OK, goodbye now.");
  return 0;
}                               /* end_of_dram_master */

/** Test function */
static MSG_error_t test_all(const char *platform_file,
                            const char *application_file)
{
  MSG_error_t res = MSG_OK;

  MSG_create_environment(platform_file);
  MSG_function_register("master", master);
  MSG_function_register("slave", slave);
  MSG_launch_application(application_file);

  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());
  return res;
}                               /* end_of_test_all */


/** Main function */
int main(int argc, char *argv[])
{
  MSG_error_t res = MSG_OK;

  MSG_global_init(&argc, argv);
  if (argc < 3) {
    XBT_CRITICAL("Usage: %s platform_file deployment_file\n", argv[0]);
    XBT_CRITICAL("example: %s msg_platform.xml msg_deployment_suspend.xml\n",
              argv[0]);
    exit(1);
  }
  test_all(argv[1], argv[2]);
  res = MSG_clean();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
