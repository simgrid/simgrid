/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "msg/msg.h"
#include "xbt/log.h"
#include "xbt/asserts.h"
#include "gras_config.h" //for HAVE_LATENCY_BOUND_TRACKING

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

int master(int argc, char *argv[]);
int slave(int argc, char *argv[]);
MSG_error_t test_all(const char *platform_file,
                     const char *application_file);

typedef enum {
  PORT_22 = 0,
  MAX_CHANNEL
} channel_t;

//keep a pointer to all surf running tasks.
#define NTASKS 1500
int bool_printed = 0;
double start_time, end_time, elapsed_time;
double gl_data_size[NTASKS];
m_task_t gl_task_array[NTASKS];
const char *slavenames[NTASKS];
const char *masternames[NTASKS];
int gl_task_array_id = 0;

#define FINALIZE ((void*)221297)        /* a magic number to tell people to stop working */

/** master */
int master(int argc, char *argv[])
{
  char *slavename = NULL;
  double task_comm_size = 0;
  m_task_t todo;
  m_host_t slave;
  char id_alias[10];
  //unique id to control statistics
  int id = -1;

  if (argc != 4) {
    INFO1("Strange number of arguments expected 3 got %d", argc - 1);
  }

  /* data size */
  xbt_assert1(sscanf(argv[1], "%lg", &task_comm_size),
              "Invalid argument %s\n", argv[1]);

  /* slave name */
  slavename = argv[2];
  id = atoi(argv[3]);
  sprintf(id_alias, "flow_%d", id);
  slavenames[id] = slavename;
  TRACE_category(id_alias);

  masternames[id] = MSG_host_get_name(MSG_host_self());

  {                             /*  Task creation.  */
    char sprintf_buffer[64] = "Task_0";
    todo = MSG_task_create(sprintf_buffer, 0, task_comm_size, NULL);
    TRACE_msg_set_task_category(todo, id_alias);
    //keep track of running tasks
    gl_task_array[id] = todo;
    gl_data_size[id] = task_comm_size;
  }

  {                             /* Process organisation */
    slave = MSG_get_host_by_name(slavename);
  }

  /* time measurement */
  sprintf(id_alias, "%d", id);
  start_time = MSG_get_clock();
  MSG_task_send(todo, id_alias);
  end_time = MSG_get_clock();


  if (!bool_printed) {
    INFO3
        ("Send completed (to %s). Transfer time: %f\t Agregate bandwidth: %f",
         slave->name, (end_time - start_time),
         task_comm_size / (end_time - start_time));
    INFO2("Completed peer: %s time: %f", slave->name,
          (end_time - start_time));
  }
  return 0;
}                               /* end_of_master */

/** Receiver function  */
int slave(int argc, char *argv[])
{

  m_task_t task = NULL;
  int a;
  int id = 0;
#ifdef HAVE_LATENCY_BOUND_TRACKING
  int limited_latency = 0;
#endif
  double remaining = 0;
  char id_alias[10];

  if (argc != 2) {
    INFO1("Strange number of arguments expected 1 got %d", argc - 1);
  }

  id = atoi(argv[1]);
  sprintf(id_alias, "%d", id);
  int trace_id = id;

  a = MSG_task_receive(&(task), id_alias);

  if (a != MSG_OK) {
    INFO0("Hey?! What's up?");
    xbt_assert0(0, "Unexpected behavior.");
  }

  elapsed_time = MSG_get_clock() - start_time;

  if (!bool_printed) {
    bool_printed = 1;
    for (id = 0; id < NTASKS; id++) {
      if (gl_task_array[id] == NULL) {
      } else if (gl_task_array[id] == task) {
#ifdef HAVE_LATENCY_BOUND_TRACKING
        limited_latency = MSG_task_is_latency_bounded(gl_task_array[id]);
        if (limited_latency) {
          INFO1("WARNING FLOW[%d] is limited by latency!!", id);
        }
#endif
        INFO5
            ("===> Estimated Bw of FLOW[%d] : %f ;  message from %s to %s  with remaining : %f",
             id, gl_data_size[id] / elapsed_time, masternames[id],
             slavenames[id], 0.0);
      } else {
        remaining =
            MSG_task_get_remaining_communication(gl_task_array[id]);
#ifdef HAVE_LATENCY_BOUND_TRACKING
        limited_latency = MSG_task_is_latency_bounded(gl_task_array[id]);

        if (limited_latency) {
          INFO1("WARNING FLOW[%d] is limited by latency!!", id);
        }
#endif
        INFO5
            ("===> Estimated Bw of FLOW[%d] : %f ;  message from %s to %s  with remaining : %f",
             id, (gl_data_size[id] - remaining) / elapsed_time,
             masternames[id], slavenames[id], remaining);
      }

    }
  }
  char mark[100];
  snprintf(mark, 100, "flow_%d_finished", trace_id);
  TRACE_mark("endmark", mark);

  MSG_task_destroy(task);
  return 0;
}                               /* end_of_slave */

/** Test function */
MSG_error_t test_all(const char *platform_file,
                     const char *application_file)
{
  MSG_error_t res = MSG_OK;

  /* MSG_config("workstation/model", "GTNETS"); */
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
  return res;
}                               /* end_of_test_all */

/** Main function */
int main(int argc, char *argv[])
{
  MSG_error_t res = MSG_OK;
  bool_printed = 0;

  MSG_global_init(&argc, argv);
  if (argc < 3) {
    printf("Usage: %s platform_file deployment_file\n", argv[0]);
    exit(1);
  }

  TRACE_start();
  TRACE_declare_mark("endmark");

  res = test_all(argv[1], argv[2]);

  MSG_clean();

  TRACE_end();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
