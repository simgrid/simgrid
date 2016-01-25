/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "simgrid/msg.h"
#include "xbt/log.h"
#include "xbt/asserts.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

/** @addtogroup MSG_examples
 * 
 * - <b>gtnets</b> Simple ping-pong using GTNeTs instead of the SimGrid network models.
 */

int master(int argc, char *argv[]);
int slave(int argc, char *argv[]);
msg_error_t test_all(const char *platform_file,
                     const char *application_file);

int timer_start = 1;

//keep a pointer to all surf running tasks.
#define NTASKS 1500
int bool_printed = 0;
double start_time, end_time, elapsed_time;
double gl_data_size[NTASKS];
msg_task_t gl_task_array[NTASKS];
const char *slavenames[NTASKS];
const char *masternames[NTASKS];
int gl_task_array_id = 0;
int count_finished = 0;

/** master */
int master(int argc, char *argv[])
{
  char *slavename = NULL;
  double task_comm_size = 0;
  msg_task_t todo;
  char id_alias[10];
  //unique id to control statistics
  int id = -1;

  xbt_assert(argc == 4, "Strange number of arguments expected 3 got %d",
             argc - 1);

  /* data size */
  int read;
  read = sscanf(argv[1], "%lg", &task_comm_size);
  xbt_assert(read, "Invalid argument %s\n", argv[1]);

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
    MSG_task_set_category(todo, id_alias);
    //keep track of running tasks
    gl_task_array[id] = todo;
    gl_data_size[id] = task_comm_size;
  }

  count_finished++;

  /* time measurement */
  sprintf(id_alias, "%d", id);
  start_time = MSG_get_clock();
  MSG_task_send(todo, id_alias);
  end_time = MSG_get_clock();


  return 0;
}                               /* end_of_master */

/** Receiver function  */
int slave(int argc, char *argv[])
{

  msg_task_t task = NULL;
  int a;
  int id = 0;
#ifdef HAVE_LATENCY_BOUND_TRACKING
  int limited_latency = 0;
#endif
  double remaining = 0;
  char id_alias[10];

  xbt_assert(argc == 2, "Strange number of arguments expected 1 got %d",
             argc - 1);

  id = atoi(argv[1]);
  sprintf(id_alias, "%d", id);
  int trace_id = id;

  a = MSG_task_receive(&(task), id_alias);

  count_finished--;
  if(count_finished == 0){
      timer_start = 0;
  }

 xbt_assert(a == MSG_OK,"Hey?! What's up? Unexpected behavior");

  elapsed_time = MSG_get_clock() - start_time;

  if (!bool_printed) {
    bool_printed = 1;

    for (id = 0; id < NTASKS; id++) {
      if (gl_task_array[id] == NULL) continue;
      if (gl_task_array[id] == task) {
#ifdef HAVE_LATENCY_BOUND_TRACKING
        limited_latency = MSG_task_is_latency_bounded(gl_task_array[id]);
        if (limited_latency) {
          XBT_INFO("WARNING FLOW[%d] is limited by latency!!", id);
        }
#endif
        XBT_INFO
            ("===> Estimated Bw of FLOW[%d] : %f ;  message from %s to %s  with remaining : %f",
             id, gl_data_size[id] / elapsed_time, masternames[id],
             slavenames[id], 0.0);
        MSG_task_destroy(gl_task_array[id]);
        gl_task_array[id]=NULL;
      } else {
        remaining =
            MSG_task_get_remaining_communication(gl_task_array[id]);
#ifdef HAVE_LATENCY_BOUND_TRACKING
        limited_latency = MSG_task_is_latency_bounded(gl_task_array[id]);

        if (limited_latency) {
          XBT_INFO("WARNING FLOW[%d] is limited by latency!!", id);
        }
#endif
        XBT_INFO
            ("===> Estimated Bw of FLOW[%d] : %f ;  message from %s to %s  with remaining : %f",
             id, (gl_data_size[id] - remaining) / elapsed_time,
             masternames[id], slavenames[id], remaining);
        if(remaining==0) {
          MSG_task_destroy(gl_task_array[id]);
          gl_task_array[id]=NULL;
        }
      }
    }
    bool_printed = 2;
  }
  char mark[100];
  snprintf(mark, 100, "flow_%d_finished", trace_id);
  TRACE_mark("endmark", mark);

  if(bool_printed==2 && gl_task_array[trace_id]) MSG_task_destroy(gl_task_array[trace_id]);

  return 0;
}                               /* end_of_slave */

/** Test function */
msg_error_t test_all(const char *platform_file,
                     const char *application_file)
{
  msg_error_t res = MSG_OK;

  {                             /*  Simulation setting */
    MSG_create_environment(platform_file);
  }

  TRACE_declare_mark("endmark");

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
  msg_error_t res = MSG_OK;
  bool_printed = 0;

  MSG_init(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
	          "\tExample: %s platform.xml deployment.xml\n", 
	          argv[0], argv[0]);
   
  res = test_all(argv[1], argv[2]);

  return res != MSG_OK;
}
   
