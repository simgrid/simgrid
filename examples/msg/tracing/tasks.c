/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "msg/msg.h"
#include "xbt/sysdep.h"         /* calloc, printf */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

int master(int argc, char *argv[]);
int slave(int argc, char *argv[]);
MSG_error_t test_all(const char *platform_file,
                     const char *application_file);

/** Emitter function  */
int master(int argc, char *argv[])
{
  long number_of_tasks = atol(argv[1]);
  double task_comp_size = atof(argv[2]);
  double task_comm_size = atof(argv[3]);
  long slaves_count = atol(argv[4]);
  XBT_INFO("master %ld %f %f %ld", number_of_tasks, task_comp_size,
        task_comm_size, slaves_count);

  int i;
  for (i = 0; i < number_of_tasks; i++) {
    char task_name[100];
    snprintf (task_name, 100, "task-%d", i);
    m_task_t task = NULL;
    task = MSG_task_create(task_name, task_comp_size, task_comm_size, NULL);

    //setting the category of task to "compute"
    //the category of a task must be defined before it is sent or executed
    TRACE_msg_set_task_category(task, "compute");
    MSG_task_send(task, "master_mailbox");
  }

  for (i = 0; i < slaves_count; i++) {
    char task_name[100];
    snprintf (task_name, 100, "task-%d", i);
    m_task_t finalize = MSG_task_create(task_name, 0, 0, xbt_strdup("finalize"));
    TRACE_msg_set_task_category(finalize, "finalize");
    MSG_task_send(finalize, "master_mailbox");
  }

  return 0;
}

/** Receiver function  */
int slave(int argc, char *argv[])
{
  m_task_t task = NULL;
  int res;

  while (1) {
    res = MSG_task_receive(&(task), "master_mailbox");
    if (res != MSG_OK) {
      XBT_INFO("error");
      break;
    }

    char *data = MSG_task_get_data(task);
    if (data && !strcmp(data, "finalize")) {
      MSG_task_destroy(task);
      break;
    }

    XBT_INFO("Executing task %f", MSG_task_get_compute_duration(task));
    MSG_task_execute(task);
    XBT_INFO("End of execution");
    MSG_task_destroy(task);
    task = NULL;
  }
  return 0;
}

/** Test function */
MSG_error_t test_all(const char *platform_file,
                     const char *application_file)
{
  MSG_error_t res = MSG_OK;

  {                             /*  Simulation setting */
    MSG_create_environment(platform_file);
  }
  {
    //declaring user categories
    TRACE_category_with_color ("compute", "1 0 0");  //compute is red
    TRACE_category_with_color ("finalize", "0 1 0"); //finalize is green
  }
  {                             /*   Application deployment */
    MSG_function_register("master", master);
    MSG_function_register("slave", slave);
    MSG_launch_application(application_file);
  }
  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());
  return res;
}


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
