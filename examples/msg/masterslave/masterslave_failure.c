/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

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
MSG_error_t test_all(const char *platform_file, const char *application_file);

typedef enum {
  PORT_22 = 0,
  MAX_CHANNEL
} channel_t;

#define FINALIZE ((void*)221297)        /* a magic number to tell people to stop working */

/** Emitter function  */
int master(int argc, char *argv[])
{
  int slaves_count = 0;
  m_host_t *slaves = NULL;
  int number_of_tasks = 0;
  double task_comp_size = 0;
  double task_comm_size = 0;


  int i;

  xbt_assert1(sscanf(argv[1], "%d", &number_of_tasks),
              "Invalid argument %s\n", argv[1]);
  xbt_assert1(sscanf(argv[2], "%lg", &task_comp_size),
              "Invalid argument %s\n", argv[2]);
  xbt_assert1(sscanf(argv[3], "%lg", &task_comm_size),
              "Invalid argument %s\n", argv[3]);

  {                             /* Process organisation */
    slaves_count = argc - 4;
    slaves = xbt_new0(m_host_t, slaves_count);

    for (i = 4; i < argc; i++) {
      slaves[i - 4] = MSG_get_host_by_name(argv[i]);
      if (slaves[i - 4] == NULL) {
        INFO1("Unknown host %s. Stopping Now! ", argv[i]);
        abort();
      }
    }
  }

  INFO1("Got %d slave(s) :", slaves_count);
  for (i = 0; i < slaves_count; i++)
    INFO1("%s", slaves[i]->name);

  INFO1("Got %d task to process :", number_of_tasks);

  for (i = 0; i < number_of_tasks; i++) {
    m_task_t task = MSG_task_create("Task", task_comp_size, task_comm_size,
                                    xbt_new0(double, 1));
    int a;
    *((double *) task->data) = MSG_get_clock();

    a =
      MSG_task_put_with_timeout(task, slaves[i % slaves_count], PORT_22,
                                10.0);
    if (a == MSG_OK) {
      INFO0("Send completed");
    } else if (a == MSG_HOST_FAILURE) {
      INFO0
        ("Gloups. The cpu on which I'm running just turned off!. See you!");
      free(slaves);
      return 0;
    } else if (a == MSG_TRANSFER_FAILURE) {
      INFO1
        ("Mmh. Something went wrong with '%s'. Nevermind. Let's keep going!",
         slaves[i % slaves_count]->name);
      MSG_task_destroy(task);
    } else if (a == MSG_TIMEOUT) {
      INFO1
        ("Mmh. Got timeouted while speaking to '%s'. Nevermind. Let's keep going!",
         slaves[i % slaves_count]->name);
      MSG_task_destroy(task);
    } else {
      INFO0("Hey ?! What's up ? ");
      xbt_assert0(0, "Unexpected behavior");
    }
  }

  INFO0
    ("All tasks have been dispatched. Let's tell everybody the computation is over.");
  for (i = 0; i < slaves_count; i++) {
    m_task_t task = MSG_task_create("finalize", 0, 0, FINALIZE);
    int a = MSG_task_put_with_timeout(task, slaves[i], PORT_22, 1.0);
    if (a == MSG_OK)
      continue;
    if (a == MSG_HOST_FAILURE) {
      INFO0
        ("Gloups. The cpu on which I'm running just turned off!. See you!");
      return 0;
    } else if (a == MSG_TRANSFER_FAILURE) {
      INFO1("Mmh. Can't reach '%s'! Nevermind. Let's keep going!",
            slaves[i]->name);
      MSG_task_destroy(task);
    } else if (a == MSG_TIMEOUT) {
      INFO1
        ("Mmh. Got timeouted while speaking to '%s'. Nevermind. Let's keep going!",
         slaves[i % slaves_count]->name);
      MSG_task_destroy(task);
    } else {
      INFO0("Hey ?! What's up ? ");
      xbt_assert2(0, "Unexpected behavior with '%s': %d", slaves[i]->name, a);
    }
  }

  INFO0("Goodbye now!");
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
    a = MSG_task_get(&(task), PORT_22);
    time2 = MSG_get_clock();
    if (a == MSG_OK) {
      INFO1("Received \"%s\"", MSG_task_get_name(task));
      if (MSG_task_get_data(task) == FINALIZE) {
        MSG_task_destroy(task);
        break;
      }
      if (time1 < *((double *) task->data))
        time1 = *((double *) task->data);
      INFO1("Communication time : \"%f\"", time2 - time1);
      INFO1("Processing \"%s\"", MSG_task_get_name(task));
      a = MSG_task_execute(task);
      if (a == MSG_OK) {
        INFO1("\"%s\" done", MSG_task_get_name(task));
        free(task->data);
        MSG_task_destroy(task);
      } else if (a == MSG_HOST_FAILURE) {
        INFO0
          ("Gloups. The cpu on which I'm running just turned off!. See you!");
        return 0;
      } else {
        INFO0("Hey ?! What's up ? ");
        xbt_assert0(0, "Unexpected behavior");
      }
    } else if (a == MSG_HOST_FAILURE) {
      INFO0
        ("Gloups. The cpu on which I'm running just turned off!. See you!");
      return 0;
    } else if (a == MSG_TRANSFER_FAILURE) {
      INFO0("Mmh. Something went wrong. Nevermind. Let's keep going!");
    } else {
      INFO0("Hey ?! What's up ? ");
      xbt_assert0(0, "Unexpected behavior");
    }
  }
  INFO0("I'm done. See you!");
  return 0;
}                               /* end_of_slave */

/** Test function */
MSG_error_t test_all(const char *platform_file, const char *application_file)
{
  MSG_error_t res = MSG_OK;

  /* MSG_config("workstation/model","KCCFLN05"); */
  {                             /*  Simulation setting */
    MSG_set_channel_number(MAX_CHANNEL);
    MSG_create_environment(platform_file);
  }
  {                             /*   Application deployment */
    MSG_function_register("master", master);
    MSG_function_register("slave", slave);
    MSG_launch_application(application_file);
  }
  res = MSG_main();

  INFO1("Simulation time %g", MSG_get_clock());
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
