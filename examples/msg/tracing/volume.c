/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "msg/msg.h"
#include "xbt/sysdep.h" /* calloc, printf */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,"Messages specific for this msg example");

int master(int argc, char *argv[]);
int slave(int argc, char *argv[]);
MSG_error_t test_all(const char *platform_file, const char *application_file);

/** Emitter function  */
int master(int argc, char *argv[])
{
  //defining the category of the master process
  TRACE_msg_set_process_category (MSG_process_self(), "master");

  long number_of_tasks = atol(argv[1]);
  long slaves_count = atol(argv[4]);
  int p = 1000000000;
  int c = 10000000;

  int i;
  for (i = 0; i < number_of_tasks; i++){
    m_task_t task = NULL;
    task = MSG_task_create ("task_compute", p, c, NULL);
    TRACE_msg_set_task_category (task, "compute");
    MSG_task_send(task, "master_mailbox");
    task = NULL;
    task = MSG_task_create ("task_request", p, c, NULL);
    TRACE_msg_set_task_category (task, "request");
    MSG_task_send(task, "master_mailbox");
    task = NULL;
    task = MSG_task_create ("task_data", p, c, NULL);
    TRACE_msg_set_task_category (task, "data");
    MSG_task_send(task, "master_mailbox");
  }
  
   for (i = 0; i < slaves_count; i++) { 
     m_task_t finalize = MSG_task_create ("finalize", 0, 1000, 0);
     TRACE_msg_set_task_category(finalize, "finalize");
     MSG_task_send(finalize, "master_mailbox");
   } 
  
  return 0;
}

/** Receiver function  */
int slave(int argc, char *argv[])
{
  //defining the category of this slave process
  TRACE_msg_set_process_category (MSG_process_self(), "slave");

  m_task_t task = NULL;
  int res;

  while(1) {
    res = MSG_task_receive(&(task), "master_mailbox");

    if (!strcmp(MSG_task_get_name(task),"finalize")) {
      MSG_task_destroy(task);
    	break;
    }

    MSG_task_execute(task);
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

  {				/*  Simulation setting */
    MSG_set_channel_number(0);
    MSG_create_environment(platform_file);
  }
  {                            /*   Application deployment */
    MSG_function_register("master", master);
    MSG_function_register("slave", slave);
    MSG_launch_application(application_file);
  }
  res = MSG_main();
  
  INFO1("Simulation time %g",MSG_get_clock());
  return res;
}


/** Main function */
int main(int argc, char *argv[])
{
  MSG_error_t res = MSG_OK;

  MSG_global_init(&argc,argv);
  if (argc < 3) {
     printf ("Usage: %s platform_file deployment_file\n",argv[0]);
     printf ("example: %s msg_platform.xml msg_deployment.xml\n",argv[0]);
     exit(1);
  }

  //starting the simulation tracing with the TRACE_VOLUME mask
  // - the communication volume among processes expects that:
  //     - the processes involved have a category
  //     - the tasks sent have a category
  TRACE_start ();

  //declaring user categories (for tasks)
  TRACE_category ("compute");
  TRACE_category ("request");
  TRACE_category ("data");
  TRACE_category ("finalize");

  //declaring user categories (for processes)
  TRACE_category ("master");
  TRACE_category ("slave");

  res = test_all(argv[1],argv[2]);
  MSG_clean();

  //ending the simulation tracing
  TRACE_end();

  if(res==MSG_OK)
    return 0;
  else
    return 1;
} /* end_of_main */
