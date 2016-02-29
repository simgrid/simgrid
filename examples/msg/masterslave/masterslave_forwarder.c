/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

/** @addtogroup MSG_examples
 * 
 *  - <b>masterslave/masterslave_forwarder.c: Master/slaves example</b>. This good old example is also very simple. Its
 *    basic version is fully commented on this page: \ref MSG_ex_master_slave, but several variants can be found in the
 *    same directory.
 */

#define FINALIZE ((void*)221297)        /* a magic number to tell people to stop working */

/** Emitter function  */
static int master(int argc, char *argv[])
{
  int slaves_count = 0;
  msg_host_t *slaves = NULL;
  msg_task_t *todo = NULL;
  int number_of_tasks = 0;
  double task_comp_size = 0;
  double task_comm_size = 0;

  int i;

  XBT_ATTRIB_UNUSED int res = sscanf(argv[1], "%d", &number_of_tasks);
  xbt_assert(res,"Invalid argument %s\n", argv[1]);
  res = sscanf(argv[2], "%lg", &task_comp_size);
  xbt_assert(res, "Invalid argument %s\n", argv[2]);
  res = sscanf(argv[3], "%lg", &task_comm_size);
  xbt_assert(res, "Invalid argument %s\n", argv[3]);

  {                             /*  Task creation */
    char sprintf_buffer[64];

    todo = xbt_new0(msg_task_t, number_of_tasks);

    for (i = 0; i < number_of_tasks; i++) {
      sprintf(sprintf_buffer, "Task_%d", i);
      todo[i] = MSG_task_create(sprintf_buffer, task_comp_size, task_comm_size, NULL);
    }
  }

  {                             /* Process organization */
    slaves_count = argc - 4;
    slaves = xbt_new0(msg_host_t, slaves_count);

    for (i = 4; i < argc; i++) {
      slaves[i - 4] = MSG_host_by_name(argv[i]);
      xbt_assert(slaves[i - 4] != NULL, "Unknown host %s. Stopping Now! ", argv[i]);
    }
  }

  XBT_INFO("Got %d slaves and %d tasks to process", slaves_count, number_of_tasks);
  for (i = 0; i < slaves_count; i++)
    XBT_DEBUG("%s", MSG_host_get_name(slaves[i]));

  for (i = 0; i < number_of_tasks; i++) {
    XBT_INFO("Sending \"%s\" to \"%s\"", todo[i]->name, MSG_host_get_name(slaves[i % slaves_count]));
    if (MSG_host_self() == slaves[i % slaves_count]) {
      XBT_INFO("Hey ! It's me ! :)");
    }

    MSG_task_send(todo[i], MSG_host_get_name(slaves[i % slaves_count]));
    XBT_INFO("Sent");
  }

  XBT_INFO("All tasks have been dispatched. Let's tell everybody the computation is over.");
  for (i = 0; i < slaves_count; i++) {
    msg_task_t finalize = MSG_task_create("finalize", 0, 0, FINALIZE);
    MSG_task_send(finalize, MSG_host_get_name(slaves[i]));
  }

  XBT_INFO("Goodbye now!");
  free(slaves);
  free(todo);
  return 0;
}

static int slave(int argc, char *argv[])
{
  msg_task_t task = NULL;
  XBT_ATTRIB_UNUSED int res;
  while (1) {
    res = MSG_task_receive(&(task),MSG_host_get_name(MSG_host_self()));
    xbt_assert(res == MSG_OK, "MSG_task_get failed");

    XBT_INFO("Received \"%s\"", MSG_task_get_name(task));
    if (!strcmp(MSG_task_get_name(task), "finalize")) {
      MSG_task_destroy(task);
      break;
    }

    XBT_INFO("Processing \"%s\"", MSG_task_get_name(task));
    MSG_task_execute(task);
    XBT_INFO("\"%s\" done", MSG_task_get_name(task));
    MSG_task_destroy(task);
    task = NULL;
  }
  XBT_INFO("I'm done. See you!");
  return 0;
}

static int forwarder(int argc, char *argv[])
{
  int i;
  int slaves_count;
  msg_host_t *slaves;

  {                             /* Process organization */
    slaves_count = argc - 1;
    slaves = xbt_new0(msg_host_t, slaves_count);

    for (i = 1; i < argc; i++) {
      slaves[i - 1] = MSG_host_by_name(argv[i]);
      if (slaves[i - 1] == NULL) {
        XBT_INFO("Unknown host %s. Stopping Now! ", argv[i]);
        abort();
      }
    }
  }

  i = 0;
  while (1) {
    msg_task_t task = NULL;
    int a;
    a = MSG_task_receive(&(task),MSG_host_get_name(MSG_host_self()));
    if (a == MSG_OK) {
      XBT_INFO("Received \"%s\"", MSG_task_get_name(task));
      if (MSG_task_get_data(task) == FINALIZE) {
        XBT_INFO("All tasks have been dispatched. Let's tell everybody the computation is over.");
        for (i = 0; i < slaves_count; i++)
          MSG_task_send(MSG_task_create("finalize", 0, 0, FINALIZE), MSG_host_get_name(slaves[i]));
        MSG_task_destroy(task);
        break;
      }
      XBT_INFO("Sending \"%s\" to \"%s\"", MSG_task_get_name(task), MSG_host_get_name(slaves[i % slaves_count]));
      MSG_task_send(task, MSG_host_get_name(slaves[i % slaves_count]));
      i++;
    } else {
      XBT_INFO("Hey ?! What's up ? ");
      xbt_die("Unexpected behavior");
    }
  }
  xbt_free(slaves);

  XBT_INFO("I'm done. See you!");
  return 0;
}

int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;

  MSG_init(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
             "\tExample: %s msg_platform.xml msg_deployment.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);

  MSG_function_register("master", master);
  MSG_function_register("slave", slave);
  MSG_function_register("forwarder", forwarder);
  MSG_launch_application(argv[2]);

  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}
