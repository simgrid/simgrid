/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

/** @addtogroup MSG_examples
 *
 *  - <b>sendrecv/sendrecv.c: Ping-pong example</b>. It's hard to think of a simpler example. The tesh files laying in
 *    the directory are instructive concerning the way to pass options to the simulators (as described in \ref options).
 */

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

double task_comm_size_lat = 1;
double task_comm_size_bw = 10e8;

static int sender(int argc, char *argv[])
{
  msg_host_t host = NULL;
  double time;
  msg_task_t task_la = NULL;
  msg_task_t task_bw = NULL;
  char sprintf_buffer_la[64];
  char sprintf_buffer_bw[64];

  XBT_INFO("sender");
  XBT_INFO("host = %s", argv[1]);

  host = MSG_host_by_name(argv[1]);

  if (host == NULL) {
    XBT_INFO("Unknown host %s. Stopping Now! ", argv[1]);
    abort();
  }

  /* Latency */
  time = MSG_get_clock();
  sprintf(sprintf_buffer_la, "latency task");
  task_la = MSG_task_create(sprintf_buffer_la, 0.0, task_comm_size_lat, NULL);
  task_la->data = xbt_new(double, 1);
  *(double *) task_la->data = time;
  XBT_INFO("task_la->data = %e", *((double *) task_la->data));
  MSG_task_send(task_la, argv[1]);

  /* Bandwidth */
  time = MSG_get_clock();
  sprintf(sprintf_buffer_bw, "bandwidth task");
  task_bw = MSG_task_create(sprintf_buffer_bw, 0.0, task_comm_size_bw, NULL);
  task_bw->data = xbt_new(double, 1);
  *(double *) task_bw->data = time;
  XBT_INFO("task_bw->data = %e", *((double *) task_bw->data));
  MSG_task_send(task_bw, argv[1]);

  return 0;
}

static int receiver(int argc, char *argv[])
{
  msg_task_t task_la = NULL;
  msg_task_t task_bw = NULL;

  XBT_INFO("receiver");

  /* Get Latency */
  int a = MSG_task_receive(&task_la,MSG_host_get_name(MSG_host_self()));
  xbt_assert(a == MSG_OK, "Unexpected behavior");

  double time1 = MSG_get_clock();
  double sender_time = *((double *) (task_la->data));
  double time = sender_time;
  double communication_time = time1 - time;
  XBT_INFO("Task received : %s", task_la->name);
  xbt_free(task_la->data);
  MSG_task_destroy(task_la);
  XBT_INFO("Communic. time %e", communication_time);
  XBT_INFO("--- la %f ----", communication_time);

  /* Get Bandwidth */
  a = MSG_task_receive(&task_bw,MSG_host_get_name(MSG_host_self()));
  xbt_assert(a == MSG_OK, "Unexpected behavior");
  time1 = MSG_get_clock();
  sender_time = *((double *) (task_bw->data));
  time = sender_time;
  communication_time = time1 - time;
  XBT_INFO("Task received : %s", task_bw->name);
  xbt_free(task_bw->data);
  MSG_task_destroy(task_bw);
  XBT_INFO("Communic. time %e", communication_time);
  XBT_INFO("--- bw %f ----", task_comm_size_bw / communication_time);

  return 0;
}

struct application {
  const char* platform_file;
  const char* application_file;
};

/** Test function */
static msg_error_t test_all(struct application* app)
{
  msg_error_t res = MSG_OK;

  MSG_create_environment(app->platform_file);

  /* Become one of the simulated process.
   *
   * This must be done after the creation of the platform because we are depending attaching to a host.*/
  MSG_process_attach("sender", NULL, MSG_host_by_name("Tremblay"), NULL);

  MSG_function_register("receiver", receiver);

  MSG_launch_application(app->application_file);

  // Execute the sender code:
  const char* argv[3] = { "sender", "Jupiter", NULL };
  sender(2, (char**) argv);

  MSG_process_detach();
  return res;
}                               /* end_of_test_all */

static void maestro(void* data)
{
  MSG_main();
}

/** Main function */
int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;

  struct application app;
  app.platform_file = argv[1];
  app.application_file = argv[2];

  SIMIX_set_maestro(maestro, &app);
  MSG_init(&argc, argv);

  if (argc != 3) {
    XBT_CRITICAL("Usage: %s platform_file deployment_file\n", argv[0]);
    xbt_die("example: %s msg_platform.xml msg_deployment.xml\n",argv[0]);
  }

  res = test_all(&app);

  XBT_INFO("Total simulation time: %e", MSG_get_clock());
  return res != MSG_OK;
}
