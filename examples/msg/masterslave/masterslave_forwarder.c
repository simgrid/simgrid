/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "msg/msg.h"            /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "xbt/sysdep.h"         /* calloc, printf */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

/** @addtogroup MSG_examples
 * 
 *  - <b>masterslave/masterslave_forwarder.c: Master/slaves
 *    example</b>. This good old example is also very simple. Its basic
 *    version is fully commented on this page: \ref MSG_ex_master_slave,
 *    but several variants can be found in the same directory. 
 */


int master(int argc, char *argv[]);
int slave(int argc, char *argv[]);
int forwarder(int argc, char *argv[]);
MSG_error_t test_all(const char *platform_file,
                     const char *application_file);

#define FINALIZE ((void*)221297)        /* a magic number to tell people to stop working */

/** Emitter function  */
int master(int argc, char *argv[])
{
  int slaves_count = 0;
  m_host_t *slaves = NULL;
  m_task_t *todo = NULL;
  int number_of_tasks = 0;
  double task_comp_size = 0;
  double task_comm_size = 0;

  int i;

  _XBT_GNUC_UNUSED int res = sscanf(argv[1], "%d", &number_of_tasks);
  xbt_assert(res,"Invalid argument %s\n", argv[1]);
  res = sscanf(argv[2], "%lg", &task_comp_size);
  xbt_assert(res, "Invalid argument %s\n", argv[2]);
  res = sscanf(argv[3], "%lg", &task_comm_size);
  xbt_assert(res, "Invalid argument %s\n", argv[3]);

  {                             /*  Task creation */
    char sprintf_buffer[64];

    todo = xbt_new0(m_task_t, number_of_tasks);

    for (i = 0; i < number_of_tasks; i++) {
      sprintf(sprintf_buffer, "Task_%d", i);
      todo[i] =
          MSG_task_create(sprintf_buffer, task_comp_size, task_comm_size,
                          NULL);
    }
  }

  {                             /* Process organisation */
    slaves_count = argc - 4;
    slaves = xbt_new0(m_host_t, slaves_count);

    for (i = 4; i < argc; i++) {
      slaves[i - 4] = MSG_get_host_by_name(argv[i]);
      xbt_assert(slaves[i - 4] != NULL, "Unknown host %s. Stopping Now! ",
                  argv[i]);
    }
  }

  XBT_INFO("Got %d slaves and %d tasks to process", slaves_count,
        number_of_tasks);
  for (i = 0; i < slaves_count; i++)
    XBT_DEBUG("%s", MSG_host_get_name(slaves[i]));

  for (i = 0; i < number_of_tasks; i++) {
    XBT_INFO("Sending \"%s\" to \"%s\"",
          todo[i]->name, MSG_host_get_name(slaves[i % slaves_count]));
    if (MSG_host_self() == slaves[i % slaves_count]) {
      XBT_INFO("Hey ! It's me ! :)");
    }

    MSG_task_send(todo[i], MSG_host_get_name(slaves[i % slaves_count]));
    XBT_INFO("Sent");
  }

  XBT_INFO
      ("All tasks have been dispatched. Let's tell everybody the computation is over.");
  for (i = 0; i < slaves_count; i++) {
    m_task_t finalize = MSG_task_create("finalize", 0, 0, FINALIZE);
    MSG_task_send(finalize, MSG_host_get_name(slaves[i]));
  }

  XBT_INFO("Goodbye now!");
  free(slaves);
  free(todo);
  return 0;
}                               /* end_of_master */

/** Receiver function  */
int slave(int argc, char *argv[])
{
  m_task_t task = NULL;
  _XBT_GNUC_UNUSED int res;
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
}                               /* end_of_slave */

/** Forwarder function */
int forwarder(int argc, char *argv[])
{
  int i;
  int slaves_count;
  m_host_t *slaves;

  {                             /* Process organisation */
    slaves_count = argc - 1;
    slaves = xbt_new0(m_host_t, slaves_count);

    for (i = 1; i < argc; i++) {
      slaves[i - 1] = MSG_get_host_by_name(argv[i]);
      if (slaves[i - 1] == NULL) {
        XBT_INFO("Unknown host %s. Stopping Now! ", argv[i]);
        abort();
      }
    }
  }

  i = 0;
  while (1) {
    m_task_t task = NULL;
    int a;
    a = MSG_task_receive(&(task),MSG_host_get_name(MSG_host_self()));
    if (a == MSG_OK) {
      XBT_INFO("Received \"%s\"", MSG_task_get_name(task));
      if (MSG_task_get_data(task) == FINALIZE) {
        XBT_INFO
            ("All tasks have been dispatched. Let's tell everybody the computation is over.");
        for (i = 0; i < slaves_count; i++)
          MSG_task_send(MSG_task_create("finalize", 0, 0, FINALIZE),
        		  MSG_host_get_name(slaves[i]));
        MSG_task_destroy(task);
        break;
      }
      XBT_INFO("Sending \"%s\" to \"%s\"",
            MSG_task_get_name(task), MSG_host_get_name(slaves[i % slaves_count]));
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
}                               /* end_of_forwarder */

/** Test function */
MSG_error_t test_all(const char *platform_file,
                     const char *application_file)
{
  MSG_error_t res = MSG_OK;

  /* MSG_config("workstation/model","KCCFLN05"); */
  {                             /*  Simulation setting */
    MSG_create_environment(platform_file);
  }
  {                             /*   Application deployment */
    MSG_function_register("master", master);
    MSG_function_register("slave", slave);
    MSG_function_register("forwarder", forwarder);
    MSG_launch_application(application_file);
  }
  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());
  return res;
}                               /* end_of_test_all */


/** Main function */
int main(int argc, char *argv[])
{
  MSG_error_t res = MSG_OK;

  MSG_global_init(&argc, argv);
  if (argc < 3) {
    printf("Usage: %s platform_file deployment_file\n", argv[0]);
    printf("example: %s msg_platform.xml msg_deployment.xml\n", argv[0]);
    exit(1);
  }
  res = test_all(argv[1], argv[2]);
  MSG_clean();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
