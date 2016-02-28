/* Copyright (c) 2007, 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

#define SLEEP(duration)                                 \
  if (MSG_process_sleep(duration) != MSG_OK)            \
    xbt_die("What's going on??? I failed to sleep!");

/** @addtogroup MSG_examples
 * 
 * - <b>suspend/suspend.c</b>: Demonstrates how to suspend and resume processes using
 *   @ref MSG_process_suspend and @ref MSG_process_resume.
 */

/** Lazy guy function. This process suspends itself asap.  */
static int lazy_guy(int argc, char *argv[])
{
  XBT_INFO("Nobody's watching me ? Let's go to sleep.");
  MSG_process_suspend(MSG_process_self());
  XBT_INFO("Uuuh ? Did somebody call me ?");

  XBT_INFO("Going to sleep...");
  SLEEP(10.0);
  XBT_INFO("Mmm... waking up.");

  XBT_INFO("Going to sleep one more time...");
  SLEEP(10.0);
  XBT_INFO("Waking up once for all!");

  XBT_INFO("Mmmh, goodbye now.");
  return 0;
}                               /* end_of_lazy_guy */

/** Dream master function. This process creates a lazy_guy process and resumes it 10 seconds later. */
static int dream_master(int argc, char *argv[])
{
  msg_process_t lazy = NULL;

  XBT_INFO("Let's create a lazy guy.");
  lazy = MSG_process_create("Lazy", lazy_guy, NULL, MSG_host_self());
  XBT_INFO("Let's wait a little bit...");
  SLEEP(10.0);
  XBT_INFO("Let's wake the lazy guy up! >:) BOOOOOUUUHHH!!!!");
  MSG_process_resume(lazy);

  SLEEP(5.0);
  XBT_INFO("Suspend the lazy guy while he's sleeping...");
  MSG_process_suspend(lazy);
  XBT_INFO("Let him finish his siesta.");
  SLEEP(10.0);
  XBT_INFO("Wake up, lazy guy!");
  MSG_process_resume(lazy);

  SLEEP(5.0);
  XBT_INFO("Suspend again the lazy guy while he's sleeping...");
  MSG_process_suspend(lazy);
  XBT_INFO("This time, don't let him finish his siesta.");
  SLEEP(2.0);
  XBT_INFO("Wake up, lazy guy!");
  MSG_process_resume(lazy);

  XBT_INFO("OK, goodbye now.");
  return 0;
}                               /* end_of_dream_master */

int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;

  MSG_init(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);
  MSG_function_register("dream_master", dream_master);
  xbt_dynar_t hosts = MSG_hosts_as_dynar();
  MSG_process_create("dream_master", dream_master, NULL, xbt_dynar_getfirst_as(hosts, msg_host_t));
  xbt_dynar_free(&hosts);
  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());
  return res != MSG_OK;
}
