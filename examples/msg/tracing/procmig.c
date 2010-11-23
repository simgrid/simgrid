/* 	$Id$	 */

/* Copyright (c) 2009 The SimGrid team. All rights reserved.                */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg/msg.h"            /* core library */
#include "xbt/sysdep.h"         /* calloc */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

/** The guy we will move from host to host. It move alone and then is moved by policeman back  */
static int emigrant(int argc, char *argv[])
{
  INFO0("Setting process category");
  TRACE_msg_set_process_category(MSG_process_self(), "emigrant");
  MSG_process_sleep(2);
  INFO0("Migrating to Tremblay");
  MSG_process_change_host(MSG_get_host_by_name("Tremblay"));
  MSG_process_sleep(2);
  INFO0("Migrating to Jupiter");
  MSG_process_change_host(MSG_get_host_by_name("Jupiter"));
  MSG_process_sleep(2);
  INFO0("Migrating to Fafard");
  MSG_process_change_host(MSG_get_host_by_name("Fafard"));
  MSG_process_sleep(2);
  INFO0("Migrating to Ginette");
  MSG_process_change_host(MSG_get_host_by_name("Ginette"));
  MSG_process_sleep(2);
  INFO0("Migrating to Bourassa");
  MSG_process_change_host(MSG_get_host_by_name("Bourassa"));
  MSG_process_sleep(2);
  return 0;
}

/** Main function */
int main(int argc, char *argv[])
{
  MSG_error_t res = MSG_OK;

  /* Argument checking */
  MSG_global_init(&argc, argv);
  if (argc < 3) {
    CRITICAL1("Usage: %s platform_file deployment_file\n", argv[0]);
    CRITICAL1("example: %s msg_platform.xml msg_deployment_suspend.xml\n",
              argv[0]);
    exit(1);
  }
  TRACE_category("emigrant");

  /* Simulation setting */
  MSG_create_environment(argv[1]);

  /* Application deployment */
  MSG_function_register("emigrant", emigrant);
  MSG_launch_application(argv[2]);

  /* Run the simulation */
  res = MSG_main();
  INFO1("Simulation time %g", MSG_get_clock());
  if (res == MSG_OK)
    res = MSG_clean();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
