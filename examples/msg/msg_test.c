/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg/msg.h"

#define VERBOSE
#include "messages.h"

int master(int argc, char *argv[]);
int slave(int argc, char *argv[]);
int forwarder(int argc, char *argv[]);
void test_all(const char *platform_file, const char *application_file);


typedef enum {
  PORT_22 = 0,
  MAX_CHANNEL
} channel_t;

void print_args(int argc, char** argv);
void print_args(int argc, char** argv)
{
  int i ; 

  fprintf(stderr,"<");
  for(i=0; i<argc; i++) 
    fprintf(stderr,"%s ",argv[i]);
  fprintf(stderr,">\n");
}

int master(int argc, char *argv[])
{
  int slaves_count = 0;
  m_host_t *slaves = NULL;
  m_task_t *todo = NULL;
  int number_of_tasks = 0;
  double task_comp_size = 0;
  double task_comm_size = 0;


  int i;

  print_args(argc,argv);

  ASSERT(sscanf(argv[1],"%d", &number_of_tasks),
	 "Invalid argument %s\n",argv[1]);
  ASSERT(sscanf(argv[2],"%lg", &task_comp_size),
	 "Invalid argument %s\n",argv[2]);
  ASSERT(sscanf(argv[3],"%lg", &task_comm_size),
	 "Invalid argument %s\n",argv[3]);

  {                  /*  Task creation */
    char sprintf_buffer[64];

    todo = calloc(number_of_tasks, sizeof(m_task_t));

    for (i = 0; i < number_of_tasks; i++) {
      sprintf(sprintf_buffer, "Task_%d", i);
      todo[i] = MSG_task_create(sprintf_buffer, task_comp_size, task_comm_size, NULL);
    }
  }

  {                  /* Process organisation */
    slaves_count = argc - 4;
    slaves = calloc(slaves_count, sizeof(m_host_t));
    
    for (i = 4; i < argc; i++) {
      slaves[i-4] = MSG_get_host_by_name(argv[i]);
      if(slaves[i-4]==NULL) {
	PRINT_MESSAGE("Unknown host %s. Stopping Now! \n", argv[i]);
	abort();
      }
    }
  }

  PRINT_MESSAGE("Got %d slave(s) :\n", slaves_count);
  for (i = 0; i < slaves_count; i++)
    PRINT_MESSAGE("\t %s\n", slaves[i]->name);

  PRINT_MESSAGE("Got %d task to process :\n", number_of_tasks);

  for (i = 0; i < number_of_tasks; i++)
    PRINT_MESSAGE("\t\"%s\"\n", todo[i]->name);

  for (i = 0; i < number_of_tasks; i++) {
    PRINT_MESSAGE("Sending \"%s\" to \"%s\"\n",
                  todo[i]->name,
                  slaves[i % slaves_count]->name);
    MSG_task_put(todo[i], slaves[i % slaves_count],
                 PORT_22);
    PRINT_MESSAGE("Send completed\n");
  }
  
  PRINT_MESSAGE("All tasks have been dispatched. Bye!\n");
  free(slaves);
  free(todo);
  return 0;
}

int slave(int argc, char *argv[])
{
  print_args(argc,argv);

  while(1) {
    m_task_t task = NULL;
    int a;
    a = MSG_task_get(&(task), PORT_22);
    if (a == MSG_OK) {
      PRINT_MESSAGE("Received \"%s\" \n", task->name);
      PRINT_MESSAGE("Processing \"%s\" \n", task->name);
      MSG_task_execute(task);
      PRINT_MESSAGE("\"%s\" done \n", task->name);
      MSG_task_destroy(task);
    } else {
      PRINT_MESSAGE("Hey ?! What's up ? \n");
      DIE("Unexpected behaviour");
    }
  }
  PRINT_MESSAGE("I'm done. See you!\n");
  return 0;
}


int forwarder(int argc, char *argv[])
{
  int i;
  int slaves_count = argc - 1;
  m_host_t *slaves = calloc(slaves_count, sizeof(m_host_t));

  print_args(argc,argv);

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


  while(1) {
    m_task_t task = NULL;
    int a;
    a = MSG_task_get(&(task), PORT_22);
    if (a == MSG_OK) {
      PRINT_MESSAGE("Received \"%s\" \n", task->name);
      PRINT_MESSAGE("Sending \"%s\" to \"%s\"\n",
		    task->name,
		    slaves[i % slaves_count]->name);
      MSG_task_put(task, slaves[i % slaves_count],
		   PORT_22);
    } else {
      PRINT_MESSAGE("Hey ?! What's up ? \n");
      DIE("Unexpected behaviour");
    }
  }

  PRINT_MESSAGE("I'm done. See you!\n");
  return 0;
}


void test_all(const char *platform_file,const char *application_file)
{
  {				/*  Simulation setting */
    MSG_set_channel_number(MAX_CHANNEL);
    MSG_create_environment(platform_file);
  }
  {                            /*   Application deployment */
    MSG_function_register("master", master);
    MSG_function_register("slave", slave);
    MSG_function_register("forwarder", forwarder);
    MSG_launch_application(application_file);
  }
  MSG_main();
  printf("Simulation time %Lg\n",MSG_getClock());
}

int main(int argc, char *argv[])
{
  MSG_global_init_args(&argc,argv);
  test_all("msg_platform.xml","msg_deployment.xml");
  MSG_clean();
  return (0);
}
