/* Copyright (c) 2007, 2009-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include "xbt.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test_exception,
                             "Messages specific for this msg example");

/** @addtogroup MSG_examples
 * 
 * - <b>exception/exception.c</b>: Demonstrates how to send an exception to a remote guy
 */

/** Victim. This process gets a lot of remote exceptions  */
static int victim(int argc, char *argv[]) {

  xbt_ex_t e;
  msg_error_t res = MSG_OK;
  
  XBT_INFO("Let's work.");
  TRY {	
    res = MSG_task_execute(MSG_task_create("Task", 1e14, 0, NULL));
    if (res != MSG_OK) {
      XBT_INFO("The MSG_task_execute caught the exception for me and returned %d)",res);
    } else {
      xbt_die("I was expecting an exception during my execution!");
    }
  } CATCH(e) {
    XBT_INFO("The received exception resumed my execution. Good. Here is it:  ----------------------->8----");
    xbt_ex_display(&e);
    XBT_INFO("(end of the first exception) ----8<------------------------");
    xbt_ex_free(e);
  }


  XBT_INFO("Let's get suspended.");
  int gotit = 0;
  TRY {
    MSG_process_suspend(MSG_process_self());
  } CATCH(e) {
    XBT_INFO("The received exception resumed my suspension. Good. Here is it:  ----------------------->8----");
    xbt_ex_display(&e);
    XBT_INFO("(end of the second exception) ----8<------------------------");
    gotit = 1;
    xbt_ex_free(e);
  }
  if(!gotit) {
    xbt_die("I was expecting an exception during my suspension!");
  }
  
  XBT_INFO("Let's sleep for 10 seconds.");
  TRY {
    res = MSG_process_sleep(10);
    if (res != MSG_OK) {
      XBT_INFO("The MSG_process_sleep caught the exception for me and returned %d)",res);
    } else {
      xbt_die("I was expecting to get an exception during my nap.");
    }
  } CATCH(e) {
    XBT_INFO("Got the second exception:   ----------------------->8----");
    xbt_ex_display(&e);
    XBT_INFO("(end of the third exception) ----8<------------------------");
    xbt_ex_free(e);
  }

  XBT_INFO("Let's try a last time to do something on something");
  MSG_process_sleep(10);

  XBT_INFO("That's enough now. I quit.");
  
  return 0;
}

/** Terrorist. This process sends a bunch of exceptions to the victim. */
static int terrorist(int argc, char *argv[])
{
  msg_process_t victim_process = NULL;

  XBT_INFO("Let's create a victim.");
  victim_process = MSG_process_create("victim", victim, NULL, MSG_host_self());

  XBT_INFO("Going to sleep for 1 second");
  if (MSG_process_sleep(1) != MSG_OK)
     xbt_die("What's going on??? I failed to sleep!");
  XBT_INFO("Send a first exception (host failure)");
  SIMIX_process_throw(victim_process, host_error, 0, "First Trick: Let's pretend that the host failed");


  XBT_INFO("Sweet, let's prepare a second trick!");
  XBT_INFO("Going to sleep for 2 seconds");

  if (MSG_process_sleep(2) != MSG_OK)
     xbt_die("What's going on??? I failed to sleep!");
  XBT_INFO("Send a second exception (host failure)");
  SIMIX_process_throw(victim_process, host_error, 0, "Second Trick: Let's pretend again that the host failed");

  XBT_INFO("Sweet, let's prepare a third trick!");
  XBT_INFO("Going to sleep for 3 seconds");

  if (MSG_process_sleep(3) != MSG_OK)
     xbt_die("What's going on??? I failed to sleep!");
  XBT_INFO("Send a third exception (cancellation)");
  SIMIX_process_throw(victim_process, cancel_error, 0, "Third Trick: Let's pretend this time that someone canceled something");

  XBT_INFO("OK, goodbye now.");
  return 0;
}

int main(int argc, char *argv[]) {
  msg_error_t res = MSG_OK;

  MSG_init(&argc, argv);
  if (argc < 3) {
    XBT_CRITICAL("Usage: %s platform_file deployment_file\n", argv[0]);
    exit(1);
  }

  MSG_function_register("terrorist", terrorist);
  MSG_create_environment(argv[1]);
  MSG_launch_application(argv[2]);

  /*
  // Simplistic platform with only one host
  sg_platf_begin();
  s_sg_platf_AS_cbarg_t AS = SG_PLATF_AS_INITIALIZER;
  sg_platf_new_AS_begin(&AS);

  s_sg_platf_host_cbarg_t host = SG_PLATF_HOST_INITIALIZER;
  host.id = "host0";
  sg_platf_new_host(&host);

  sg_platf_new_AS_end();
  sg_platf_end();

  // Add one process -- super heavy just to launch an application!
  SIMIX_init_application();
  sg_platf_begin();

  s_sg_platf_process_cbarg_t process = SG_PLATF_PROCESS_INITIALIZER;
  process.argc=1;
  process.argv = malloc(sizeof(char*)*2);
  process.argv[0] = "terrorist";
  process.argv[1] = NULL;
  process.host = "host0";
  process.function = "terrorist";
  process.start_time = 0;
  sg_platf_new_process(&process);
  sg_platf_end();
*/

  // Launch the simulation
  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());
  if (res == MSG_OK)
    return 0;
  else
    return 1;
}
