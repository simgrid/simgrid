/* Copyright (c) 2007-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

#include <stdio.h> /* snprintf */

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

int timer_start; //set as 1 in the master process

//keep a pointer to all surf running tasks.
#define NTASKS 1500
double start_time, end_time, elapsed_time;
double gl_data_size[NTASKS];
msg_task_t gl_task_array[NTASKS];
const char *workernames[NTASKS];
const char *masternames[NTASKS];
int gl_task_array_id = 0;
int count_finished = 0;

#define FINALIZE ((void*)221297)        /* a magic number to tell people to stop working */

static int master(int argc, char *argv[])
{
  msg_task_t todo;

  xbt_assert(argc==4,"Strange number of arguments expected 3 got %d", argc - 1);

  XBT_DEBUG ("Master started");

  /* data size */
  double task_comm_size = xbt_str_parse_double(argv[1], "Invalid task communication size: %s");

  /* worker name */
  char *workername = argv[2];
  int id = xbt_str_parse_int(argv[3], "Invalid ID as argument 3: %s");   //unique id to control statistics
  char *id_alias = xbt_malloc(20*sizeof(char));
  snprintf(id_alias, 20, "flow_%d", id);
  workernames[id] = workername;
  TRACE_category(id_alias);

  masternames[id] = MSG_host_get_name(MSG_host_self());

  {                             /*  Task creation.  */
    todo = MSG_task_create("Task_0", 100*task_comm_size, task_comm_size, NULL);
    MSG_task_set_category(todo, id_alias);
    //keep track of running tasks
    gl_task_array[id] = todo;
    gl_data_size[id] = task_comm_size;
  }

  MSG_host_by_name(workername);

  count_finished++;
  timer_start = 1 ;

  /* time measurement */
  snprintf(id_alias,20,"%d", id);
  start_time = MSG_get_clock();
  MSG_task_send(todo, id_alias);
  end_time = MSG_get_clock();

  XBT_DEBUG ("Finished");
  xbt_free(id_alias);
  return 0;
}

static int timer(int argc, char *argv[])
{
  double sleep_time;
  double first_sleep;

  xbt_assert(argc==3,"Strange number of arguments expected 2 got %d", argc - 1);

  sscanf(argv[1], "%lf", &first_sleep);
  sscanf(argv[2], "%lf", &sleep_time);

  XBT_DEBUG ("Timer started");

  if(first_sleep){
      MSG_process_sleep(first_sleep);
  }

  do {
    XBT_DEBUG ("Get sleep");
    MSG_process_sleep(sleep_time);
  } while(timer_start);

  XBT_DEBUG ("Finished");
  return 0;
}

static int worker(int argc, char *argv[])
{
  msg_task_t task = NULL;
  char id_alias[10];

  xbt_assert(argc==2,"Strange number of arguments expected 1 got %d", argc - 1);

  XBT_DEBUG ("Worker started");

  int id = xbt_str_parse_int(argv[1], "Invalid id: %s");
  snprintf(id_alias,10, "%d", id);

  msg_error_t a = MSG_task_receive(&(task), id_alias);

  count_finished--;
  if(count_finished == 0){
      timer_start = 0;
  }

  xbt_assert(a == MSG_OK,"Hey?! What's up? Unexpected behavior");

  elapsed_time = MSG_get_clock() - start_time;

  XBT_INFO("FLOW[%d] : Receive %.0f bytes from %s to %s", id, MSG_task_get_bytes_amount(task), masternames[id],
           workernames[id]);
//  MSG_task_execute(task);
  MSG_task_destroy(task);

  XBT_DEBUG ("Finished");
  return 0;
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  xbt_assert(argc > 2, "Usage: %s platform_file deployment_file\n"
             "\tExample: %s platform.xml deployment.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);
  TRACE_declare_mark("endmark");

  MSG_function_register("master", master);
  MSG_function_register("worker", worker);
  MSG_function_register("timer", timer);

  MSG_launch_application(argv[2]);

  msg_error_t res = MSG_main();

  return res != MSG_OK;
}
