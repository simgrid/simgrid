/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

#include <stdio.h> /* snprintf */

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_app_pingpong, "Messages specific for this msg example");

static int pinger(int argc, char* argv[])
{
  xbt_assert(argc == 2, "The pinger function one argument from the XML deployment file");
  XBT_INFO("Ping -> %s", argv[1]);
  xbt_assert(MSG_host_by_name(argv[1]) != NULL, "Unknown host %s. Stopping Now! ", argv[1]);

  /* - Do the ping with a 1-Byte task (latency bound) ... */
  double now                = MSG_get_clock();
  msg_task_t ping_task      = MSG_task_create("small communication (latency bound)", 0.0, 1, &now);
  MSG_task_send(ping_task, argv[1]);

  /* - ... then wait for the (large) pong */
  msg_task_t pong_task = NULL;
  int a                = MSG_task_receive(&pong_task, MSG_host_get_name(MSG_host_self()));
  xbt_assert(a == MSG_OK, "Unexpected behavior");

  double sender_time        = *((double*)(MSG_task_get_data(pong_task)));
  double communication_time = MSG_get_clock() - sender_time;
  XBT_INFO("Task received : %s", MSG_task_get_name(pong_task));
  MSG_task_destroy(pong_task);
  XBT_INFO("Pong time (bandwidth bound): %.3f", communication_time);

  return 0;
}

static int ponger(int argc, char* argv[])
{
  xbt_assert(argc == 2, "The ponger function one argument from the XML deployment file");
  XBT_INFO("Pong -> %s", argv[1]);
  xbt_assert(MSG_host_by_name(argv[1]) != NULL, "Unknown host %s. Stopping Now! ", argv[1]);

  /* - Receive the (small) ping first ....*/
  msg_task_t ping_task = NULL;
  int a                = MSG_task_receive(&ping_task, MSG_host_get_name(MSG_host_self()));
  xbt_assert(a == MSG_OK, "Unexpected behavior");

  double sender_time        = *((double*)(MSG_task_get_data(ping_task)));
  double communication_time = MSG_get_clock() - sender_time;
  XBT_INFO("Task received : %s", MSG_task_get_name(ping_task));
  MSG_task_destroy(ping_task);
  XBT_INFO(" Ping time (latency bound) %f", communication_time);

  /*  - ... Then send a 1GB pong back (bandwidth bound) */
  double now                = MSG_get_clock();
  msg_task_t pong_task      = MSG_task_create("large communication (bandwidth bound)", 0.0, 1e9, &now);
  XBT_INFO("task_bw->data = %.3f", *((double*)MSG_task_get_data(pong_task)));
  MSG_task_send(pong_task, argv[1]);

  return 0;
}

int main(int argc, char* argv[])
{
  MSG_init(&argc, argv);

  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
                       "\tExample: %s ../../platforms/small_platform.xml app-pingpong_d.xml\n",
             argv[0], argv[0]);

  MSG_create_environment(argv[1]); /* - Load the platform description */

  MSG_function_register("pinger", pinger); /* - Register the functions to be executed by the processes */
  MSG_function_register("ponger", ponger);

  MSG_launch_application(argv[2]); /* - Deploy the application */

  msg_error_t res = MSG_main(); /* - Run the simulation */

  XBT_INFO("Total simulation time: %.3f", MSG_get_clock());
  return res != MSG_OK;
}
