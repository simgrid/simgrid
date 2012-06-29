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
  msg_host_t *slaves = NULL;
  int number_of_tasks = 0;
  double task_comp_size = 0;
  double task_comm_size = 0;
  int i;
  _XBT_GNUC_UNUSED int read;

  read = sscanf(argv[1], "%d", &number_of_tasks);
  xbt_assert(read, "Invalid argument %s\n", argv[1]);
  read = sscanf(argv[2], "%lg", &task_comp_size);
  xbt_assert(read, "Invalid argument %s\n", argv[2]);
  read = sscanf(argv[3], "%lg", &task_comm_size);
  xbt_assert(read, "Invalid argument %s\n", argv[3]);

  {                             /* Process organisation */
    slaves_count = argc - 4;
    slaves = xbt_new0(msg_host_t, slaves_count);

    for (i = 4; i < argc; i++) {
      slaves[i - 4] = MSG_get_host_by_name(argv[i]);
      if (slaves[i - 4] == NULL) {
        XBT_INFO("Unknown host %s. Stopping Now! ", argv[i]);
        abort();
      }
    }
  }

  XBT_INFO("Got %d slave(s) :", slaves_count);
  for (i = 0; i < slaves_count; i++)
    XBT_INFO("%s", MSG_host_get_name(slaves[i]));

  XBT_INFO("Got %d task to process :", number_of_tasks);

  for (i = 0; i < number_of_tasks; i++) {
    m_task_t task = MSG_task_create("Task", task_comp_size, task_comm_size,
                                    xbt_new0(double, 1));
    int a;
    *((double *) task->data) = MSG_get_clock();

    a = MSG_task_send_with_timeout(task,MSG_host_get_name(slaves[i % slaves_count]),10.0);

    if (a == MSG_OK) {
      XBT_INFO("Send completed");
    } else if (a == MSG_HOST_FAILURE) {
      XBT_INFO
          ("Gloups. The cpu on which I'm running just turned off!. See you!");
      free(task->data);
      MSG_task_destroy(task);
      free(slaves);
      return 0;
    } else if (a == MSG_TRANSFER_FAILURE) {
      XBT_INFO
          ("Mmh. Something went wrong with '%s'. Nevermind. Let's keep going!",
              MSG_host_get_name(slaves[i % slaves_count]));
      free(task->data);
      MSG_task_destroy(task);
    } else if (a == MSG_TIMEOUT) {
      XBT_INFO
          ("Mmh. Got timeouted while speaking to '%s'. Nevermind. Let's keep going!",
              MSG_host_get_name(slaves[i % slaves_count]));
      free(task->data);
      MSG_task_destroy(task);
    } else {
      XBT_INFO("Hey ?! What's up ? ");
      xbt_die( "Unexpected behavior");
    }
  }

  XBT_INFO
      ("All tasks have been dispatched. Let's tell everybody the computation is over.");
  for (i = 0; i < slaves_count; i++) {
    m_task_t task = MSG_task_create("finalize", 0, 0, FINALIZE);
    int a = MSG_task_send_with_timeout(task,MSG_host_get_name(slaves[i]),1.0);
    if (a == MSG_OK)
      continue;
    if (a == MSG_HOST_FAILURE) {
      XBT_INFO
          ("Gloups. The cpu on which I'm running just turned off!. See you!");
      MSG_task_destroy(task);
      free(slaves);
      return 0;
    } else if (a == MSG_TRANSFER_FAILURE) {
      XBT_INFO("Mmh. Can't reach '%s'! Nevermind. Let's keep going!",
          MSG_host_get_name(slaves[i]));
      MSG_task_destroy(task);
    } else if (a == MSG_TIMEOUT) {
      XBT_INFO
          ("Mmh. Got timeouted while speaking to '%s'. Nevermind. Let's keep going!",
              MSG_host_get_name(slaves[i % slaves_count]));
      MSG_task_destroy(task);
    } else {
      XBT_INFO("Hey ?! What's up ? ");
      xbt_die("Unexpected behavior with '%s': %d", MSG_host_get_name(slaves[i]), a);
    }
  }

  XBT_INFO("Goodbye now!");
  free(slaves);
  return 0;
}                               /* end_of_master */

/** Receiver function  */
int slave(int argc, char *argv[])
{
  while (1) {
    m_task_t task = NULL;
    int a;
    double time1, time2;

    time1 = MSG_get_clock();
    a = MSG_task_receive( &(task), MSG_host_get_name(MSG_host_self()) );
    time2 = MSG_get_clock();
    if (a == MSG_OK) {
      XBT_INFO("Received \"%s\"", MSG_task_get_name(task));
      if (MSG_task_get_data(task) == FINALIZE) {
        MSG_task_destroy(task);
        break;
      }
      if (time1 < *((double *) task->data))
        time1 = *((double *) task->data);
      XBT_INFO("Communication time : \"%f\"", time2 - time1);
      XBT_INFO("Processing \"%s\"", MSG_task_get_name(task));
      a = MSG_task_execute(task);
      if (a == MSG_OK) {
        XBT_INFO("\"%s\" done", MSG_task_get_name(task));
        free(task->data);
        MSG_task_destroy(task);
      } else if (a == MSG_HOST_FAILURE) {
        XBT_INFO
            ("Gloups. The cpu on which I'm running just turned off!. See you!");
        return 0;
      } else {
        XBT_INFO("Hey ?! What's up ? ");
        xbt_die("Unexpected behavior");
      }
    } else if (a == MSG_HOST_FAILURE) {
      XBT_INFO
          ("Gloups. The cpu on which I'm running just turned off!. See you!");
      return 0;
    } else if (a == MSG_TRANSFER_FAILURE) {
      XBT_INFO("Mmh. Something went wrong. Nevermind. Let's keep going!");
    } else {
      XBT_INFO("Hey ?! What's up ? ");
      xbt_die("Unexpected behavior");
    }
  }
  XBT_INFO("I'm done. See you!");
  return 0;
}                               /* end_of_slave */

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

  MSG_init(&argc, argv);
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
