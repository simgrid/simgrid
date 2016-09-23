/* Copyright (c) 2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* In this test, we have two senders sending one message to a common receiver.
 * The receiver should be able to see any ordering between the two messages.
 * If we model-check the application with assertions on a specific order of
 * the messages (see the assertions in the receiver code), it should fail
 * because both ordering are possible.
 *
 * If the senders sends the message directly, the current version of the MC
 * finds that the ordering may differ and the MC find a counter-example.
 *
 * However, if the senders send the message in a mutex, the MC always let
 * the first process take the mutex because it thinks that the effect of
 * a mutex is purely local: the ordering of the messages is always the same
 * and the MC does not find the counter-example.
 */

#include "simgrid/msg.h"
#include "mc/mc.h"
#include <xbt/synchro_core.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

#define BOX_NAME "box"

#ifndef DISABLE_THE_MUTEX
static xbt_mutex_t mutex = NULL;
#endif

static int receiver(int argc, char *argv[])
{
  msg_task_t task = NULL;

  MSG_task_receive(&task, BOX_NAME);
  MC_assert(strcmp(MSG_task_get_name(task), "X") == 0);
  MSG_task_destroy(task);

  MSG_task_receive(&task, BOX_NAME);
  MC_assert(strcmp(MSG_task_get_name(task), "Y") == 0);
  MSG_task_destroy(task);

  return 0;
}

static int sender(int argc, char *argv[])
{
  char* message_name = argv[1];
#ifndef DISABLE_THE_MUTEX
  xbt_mutex_acquire(mutex);
#endif
  MSG_task_send(MSG_task_create(message_name, 0.0, 0.0, NULL), BOX_NAME);
#ifndef DISABLE_THE_MUTEX
  xbt_mutex_release(mutex);
#endif
  return 0;
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
       "\tExample: %s msg_platform.xml msg_deployment.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);
  MSG_function_register("receiver", receiver);
  MSG_function_register("sender", sender);

  MSG_launch_application(argv[2]);
#ifndef DISABLE_THE_MUTEX
  mutex = xbt_mutex_init();
#endif
  msg_error_t res = MSG_main();
#ifndef DISABLE_THE_MUTEX
  xbt_mutex_destroy(mutex); mutex = NULL;
#endif
  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}
