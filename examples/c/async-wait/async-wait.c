/* Copyright (c) 2010-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/comm.h"
#include "simgrid/engine.h"
#include "simgrid/mailbox.h"

#include "xbt/asserts.h"
#include "xbt/log.h"
#include "xbt/str.h"

#include <stdio.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(async_wait, "Messages specific for this example");

/* Main function of the Sender process */
static void sender(int argc, char* argv[])
{
  xbt_assert(argc == 5, "The sender function expects 4 arguments from the XML deployment file");
  long messages_count     = xbt_str_parse_int(argv[1], "Invalid amount of messages: %s");  /* - number of messages */
  double message_size     = xbt_str_parse_double(argv[2], "Invalid message size: %s");     /* - communication cost */
  double sleep_start_time = xbt_str_parse_double(argv[3], "Invalid sleep start time: %s"); /* - start time */
  double sleep_test_time  = xbt_str_parse_double(argv[4], "Invalid test time: %s");        /* - test time */

  XBT_INFO("sleep_start_time : %f , sleep_test_time : %f", sleep_start_time, sleep_test_time);
  sg_mailbox_t mailbox = sg_mailbox_by_name("receiver");

  sg_actor_sleep_for(sleep_start_time);
  for (int i = 0; i < messages_count; i++) {
    char* payload = bprintf("Message %d", i);

    /* This actor first sends a message asynchronously with @ref sg_mailbox_put_async. Then, if: */
    sg_comm_t comm = sg_mailbox_put_async(mailbox, payload, message_size);
    XBT_INFO("Send '%s' to 'receiver'", payload);

    if (sleep_test_time > 0) {          /* - "test_time" is set to 0, wait on @ref sg_comm_wait */
      while (sg_comm_test(comm) == 0) { /* - Call @ref sg_comm_test every "sleep_test_time" otherwise */
        sg_actor_sleep_for(sleep_test_time);
      }
    } else {
      sg_comm_wait(comm);
    }
  }

  sg_comm_t comm = sg_mailbox_put_async(mailbox, xbt_strdup("finalize"), 0);
  XBT_INFO("Send 'finalize' to 'receiver'");

  if (sleep_test_time > 0) {
    while (sg_comm_test(comm) == 0) {
      sg_actor_sleep_for(sleep_test_time);
    }
  } else {
    sg_comm_wait(comm);
  }
}

/* Receiver process expects 3 arguments: */
static void receiver(int argc, char* argv[])
{
  xbt_assert(argc == 3, "The relay_runner function does not accept any parameter from the XML deployment file");
  double sleep_start_time = xbt_str_parse_double(argv[1], "Invalid sleep start parameter: %s"); /* - start time */
  double sleep_test_time  = xbt_str_parse_double(argv[2], "Invalid sleep test parameter: %s");  /* - test time */
  XBT_INFO("sleep_start_time : %f , sleep_test_time : %f", sleep_start_time, sleep_test_time);

  sg_actor_sleep_for(sleep_start_time); /* This actor first sleeps for "start time" seconds.  */

  sg_mailbox_t mailbox = sg_mailbox_by_name("receiver");
  void* received       = NULL;

  XBT_INFO("Wait for my first message");
  while (1) {
    /* Then it posts asynchronous receives (@ref sg_mailbox_get_async) and*/
    sg_comm_t comm = sg_mailbox_get_async(mailbox, &received);

    if (sleep_test_time > 0) {          /* - if "test_time" is set to 0, wait on @ref sg_comm_wait */
      while (sg_comm_test(comm) == 0) { /* - Call @ref sg_comm_test every "sleep_test_time" otherwise */
        sg_actor_sleep_for(sleep_test_time);
      }
    } else {
      sg_comm_wait(comm);
    }
    XBT_INFO("I got a '%s'.", (char*)received);

    if (strcmp((char*)received, "finalize") == 0) { /* If the received task is "finalize", the actor ends */
      free(received);
      break;
    }
    free(received);
  }
}

int main(int argc, char* argv[])
{
  simgrid_init(&argc, argv);
  xbt_assert(argc > 2,
             "Usage: %s platform_file deployment_file\n"
             "\tExample: %s msg_platform.xml msg_deployment.xml\n",
             argv[0], argv[0]);

  simgrid_load_platform(argv[1]); /* - Load the platform description */

  simgrid_register_function("sender", sender);
  simgrid_register_function("receiver", receiver);
  simgrid_load_deployment(argv[2]); /* - Deploy the sender and receiver actors */

  simgrid_run(); /* - Run the simulation */

  return 0;
}
