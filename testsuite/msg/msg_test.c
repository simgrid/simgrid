/* 	$Id$	 */

/* Copyright (c) 2002,2004,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** \file msg_test.c 
 *  \brief Test program for msg.
*/

#include "msg/msg.h"

/** This flag enable the debugging messages from PRINT_DEBUG_MESSAGE() */
#undef VERBOSE
#include "messages.h"

int unix_emitter(int argc, char *argv[]);
int unix_receiver(int argc, char *argv[]);
void test_all(const char *platform_file, const char *application_file, double sharing);


/** The names of the channels we will use in this simulation. There is
    only one channel identified by the name PORT_22. */
typedef enum {
  PORT_22 = 0,
  MAX_CHANNEL
} channel_t;

/** The number of task each slave will process */
#define NB_TASK 3
int unix_emitter(int argc, char *argv[])
{
  int slaves_count = 0;
  m_host_t *slaves = NULL;
  int todo_count = 0;
  m_task_t *todo = NULL;

  int i;

  {                  /* Process organisation */
    slaves_count = argc - 1;
    slaves = calloc(slaves_count, sizeof(m_host_t));
    
    for (i = 1; i < argc; i++) {
      slaves[i-1] = MSG_get_host_by_name(argv[i]);
      if(slaves[i-1]==NULL) {
	PRINT_MESSAGE("Unknown host %s. Stopping Now! \n", argv[i]);
	abort();
      }
    }
  }

  {                  /*  Task creation */
    char sprintf_buffer[64];
    int slave  = slaves_count;

    todo = calloc(NB_TASK * slave, sizeof(m_task_t));
    todo_count = NB_TASK * slave;

    for (i = 0; i < NB_TASK * slave; i++) {
      sprintf(sprintf_buffer, "Task_%d", i);
      todo[i] = MSG_task_create(sprintf_buffer, 5000, 10, NULL);
    }
  }

  PRINT_MESSAGE("Got %d slave(s) :\n", slaves_count);
  for (i = 0; i < slaves_count; i++)
    PRINT_MESSAGE("\t %s\n", slaves[i]->name);

  PRINT_MESSAGE("Got %d task to process :\n", todo_count);

  for (i = 0; i < todo_count; i++)
    PRINT_MESSAGE("\t\"%s\"\n", todo[i]->name);

  for (i = 0; i < todo_count; i++) {
    PRINT_MESSAGE("Sending \"%s\" to \"%s\"\n",
                  todo[i]->name,
                  slaves[i % slaves_count]->name);
    MSG_task_put(todo[i], slaves[i % slaves_count],
                 PORT_22);
    PRINT_MESSAGE("Send completed\n");
  }
  
  free(slaves);
  free(todo);
  return 0;
}

int unix_receiver(int argc, char *argv[])
{
  int todo_count = 0;
  m_task_t *todo = (m_task_t *) calloc(NB_TASK, sizeof(m_task_t));
  int i;


  for (i = 0; i < NB_TASK;) {
    int a;
    PRINT_MESSAGE("Awaiting Task %d \n", i);
    a = MSG_task_get(&(todo[i]), PORT_22);
    if (a == MSG_OK) {
      todo_count++;
      PRINT_MESSAGE("Received \"%s\" \n", todo[i]->name);
      PRINT_MESSAGE("Processing \"%s\" \n", todo[i]->name);
      MSG_task_execute(todo[i]);
      PRINT_MESSAGE("\"%s\" done \n", todo[i]->name);
      MSG_task_destroy(todo[i]);
      i++;
    } else {
      PRINT_MESSAGE("Hey ?! What's up ? \n");
      DIE("Unexpected behaviour");
    }
  }
  free(todo);
  return 0;
}


void test_all(const char *platform_file,const char *application_file, double sharing)
{
  {				/*  Simulation setting */
    MSG_global_init();
    MSG_set_verbosity(MSG_SILENT);
    MSG_set_channel_number(MAX_CHANNEL);
    if(sharing<=0) {
      MSG_set_sharing_policy(MSG_TCP,.1);
    } else {
      MSG_set_sharing_policy(MSG_STORE_AND_FORWARD,sharing);
    }
    MSG_create_environment(platform_file);
  }
  {                            /*   Application deployment */
    MSG_function_register("master", unix_emitter);
    MSG_function_register("slave", unix_receiver);
    MSG_launch_application(application_file);
  }
  MSG_main();
/*   printf("Simulation time %g\n",MSG_getClock()); */
  MSG_clean();
}

int main(int argc, char *argv[])
{
  test_all("msg_platform.txt","msg_deployment.txt",-.1);
  test_all("msg_platform.txt","msg_deployment.txt",.1);
  return (0);
}
