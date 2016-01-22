/* Copyright (c) 2007, 2009-2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"        /* Yeah! If you want to use msg, you need to include simgrid/msg.h */
#include "xbt/sysdep.h"         /* calloc */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

static int my_onexit(void* ignored1, void *ignored2) {
  XBT_INFO("Exiting now (done sleeping or got killed).");
  return 0;
}

static int sleeper(int argc, char *argv[])
{
  XBT_INFO("Hello! I go to sleep.");
  MSG_process_on_exit(my_onexit, NULL);
   
  MSG_process_sleep(atoi(argv[1]));
  XBT_INFO("Done sleeping.");
  return 0;
}

/** Test function */
static msg_error_t test_all(const char *platform_file,
                            const char *application_file)
{
  msg_error_t res = MSG_OK;

  MSG_create_environment(platform_file);
  MSG_function_register("sleeper", sleeper);
  MSG_launch_application(application_file);

  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());
  return res;
}                               /* end_of_test_all */


/** Main function */
int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;

  MSG_init(&argc, argv);
   xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
	         "\tExample: %s msg_platform.xml msg_deployment.xml\n", 
	         argv[0], argv[0]);
  
  test_all(argv[1], argv[2]);

  return res != MSG_OK;
}
