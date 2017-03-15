/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

#define FINALIZE ((void*)221297)        /* a magic number to tell people to stop working */

static char * build_channel_name(char *buffer, const char *sender, const char* receiver)
{
  strcpy(buffer, sender);
  strcat(buffer, ":");
  strcat(buffer, receiver);
  return buffer;
}

/* forward definitions */
static int master(int argc, char* argv[]);
static int worker(int argc, char* argv[]);

static int master(int argc, char *argv[])
{
  msg_host_t host_self = MSG_host_self();
  char *master_name = (char *) MSG_host_get_name(host_self);
  char channel[1024];

  double timeout   = xbt_str_parse_double(argv[1], "Invalid timeout: %s");             /** - timeout      */
  double comp_size = xbt_str_parse_double(argv[2], "Invalid computational size: %s");  /** - Task compute cost    */
  double comm_size = xbt_str_parse_double(argv[3], "Invalid communication size: %s");  /** - Task communication size */

  /* Get the info about the worker processes (directly from SimGrid) */
  int workers_count   = argc - 4;
  msg_host_t* workers = xbt_dynar_to_array(MSG_hosts_as_dynar());

  for (int i = 0; i < workers_count; i++) // Remove my host from the list
    if (host_self == workers[i]) {
      workers[i] = workers[workers_count - 1];
      workers_count--;
      break;
    }

  for (int i = 0; i < workers_count; i++)
    MSG_process_create("worker", worker, (void*)master_name, workers[i]);
  XBT_INFO("Got %d workers and will send tasks for %g seconds", workers_count, timeout);

  /* Dispatch the tasks */
  int task_num = 0;
  while (1) {
    if (MSG_get_clock() > timeout)
      break;

    char sprintf_buffer[64];
    sprintf(sprintf_buffer, "Task_%d", task_num);
    msg_task_t task = MSG_task_create(sprintf_buffer, comp_size, comm_size, NULL);

    build_channel_name(channel, master_name, MSG_host_get_name(workers[task_num % workers_count]));

    XBT_DEBUG("Sending '%s' to channel '%s'", task->name, channel);
    MSG_task_send(task, channel);
    XBT_DEBUG("Sent");
    task_num++;
  }


  XBT_DEBUG ("All tasks have been dispatched. Let's tell everybody the computation is over.");
  for (int i = 0; i < workers_count; i++) {
    msg_task_t finalize = MSG_task_create("finalize", 0, 0, FINALIZE);
    MSG_task_send(finalize, build_channel_name(channel,master_name,
        MSG_host_get_name(workers[i % workers_count])));
  }

  XBT_INFO("Sent %d tasks in total!", task_num);
  free(workers);
  return 0;
}

/** Worker function  */
static int worker(int argc, char *argv[])
{
  char channel[1024];

  build_channel_name(channel,MSG_process_get_data(MSG_process_self()),  MSG_host_get_name(MSG_host_self()));

  XBT_DEBUG("Receiving on channel '%s'", channel);

  while (1) {
    msg_task_t task = NULL;
    int res = MSG_task_receive(&(task), channel);
    xbt_assert(res == MSG_OK, "MSG_task_receive failed");

    XBT_DEBUG("Received '%s'", MSG_task_get_name(task));
    if (!strcmp(MSG_task_get_name(task), "finalize")) {
      MSG_task_destroy(task);
      break;
    }

    XBT_DEBUG("Processing '%s'", MSG_task_get_name(task));
    MSG_task_execute(task);
    XBT_DEBUG("'%s' done", MSG_task_get_name(task));
    MSG_task_destroy(task);
  }
  XBT_DEBUG("I'm done. See you!");
  return 0;
}

/** Main function */
int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
             "\tExample: %s msg_platform.xml msg_deployment.xml\n", argv[0], argv[0]);

  /*  Create a simulated platform */
  MSG_create_environment(argv[1]);

  /*   Application deployment */
  MSG_function_register("master", master);
  MSG_function_register("worker", worker);
  MSG_launch_application(argv[2]);

  /* Run the simulation */
  msg_error_t res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());
  return (res != MSG_OK);
}
