/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** \file msg_test.c 
 *  \ingroup MSG_examples
 *  \brief Simulation of a master-slave application using a realistic platform 
 *  and an external description of the deployment.
*/

/** Yeah! If you want to use msg, you need to include msg/msg.h */
#include "msg/msg.h"

/** This includes creates a log channel for to have nice outputs. */
#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,"Messages specific for this msg example");

int master(int argc, char *argv[]);
int slave(int argc, char *argv[]);
int forwarder(int argc, char *argv[]);
void test_all(const char *platform_file, const char *application_file);


typedef enum {
  PORT_22 = 0,
  MAX_CHANNEL
} channel_t;

/** Print arguments
 * This function is just used so that users can check that each process
 *  has received the arguments it was supposed to receive.
 */
static void print_args(int argc, char** argv)
{
  int i ; 

  fprintf(stderr,"<");
  for(i=0; i<argc; i++) 
    fprintf(stderr,"%s ",argv[i]);
  fprintf(stderr,">\n");
}

/** Emitter function
 * This function has to be assigned to a m_process_t that will behave as the master.
   It should not be called directly but either given as a parameter to
   #MSG_process_create() or registered as a public function through 
   #MSG_function_register() and then automatically assigned to a process through
   #MSG_launch_application().
 
   C style arguments (argc/argv) are interpreted as 
   \li the number of tasks to distribute
   \li the computation size of each task
   \li the size of the files associated to each task
   \li a list of host that will accept those tasks.

   Tasks are dumbly sent in a round-robin style.
  */
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

  xbt_assert1(sscanf(argv[1],"%d", &number_of_tasks),
	 "Invalid argument %s\n",argv[1]);
  xbt_assert1(sscanf(argv[2],"%lg", &task_comp_size),
	 "Invalid argument %s\n",argv[2]);
  xbt_assert1(sscanf(argv[3],"%lg", &task_comm_size),
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
	INFO1("Unknown host %s. Stopping Now! ", argv[i]);
	abort();
      }
    }
  }

  INFO1("Got %d slave(s) :", slaves_count);
  for (i = 0; i < slaves_count; i++)
    INFO1("\t %s", slaves[i]->name);

  INFO1("Got %d task to process :", number_of_tasks);

  for (i = 0; i < number_of_tasks; i++)
    INFO1("\t\"%s\"", todo[i]->name);

  for (i = 0; i < number_of_tasks; i++) {
    INFO2("Sending \"%s\" to \"%s\"",
                  todo[i]->name,
                  slaves[i % slaves_count]->name);
    MSG_task_put(todo[i], slaves[i % slaves_count],
                 PORT_22);
    INFO0("Send completed");
  }
  
  INFO0("All tasks have been dispatched. Bye!");
  free(slaves);
  free(todo);
  return 0;
}

/** Receiver function
 * This function has to be assigned to a #m_process_t that has to behave as a slave.
   Just like #master(), it should not be called directly.

   This function keeps waiting for tasks and executes them as it receives them.
  */
int slave(int argc, char *argv[])
{
  print_args(argc,argv);

  while(1) {
    m_task_t task = NULL;
    int a;
    a = MSG_task_get(&(task), PORT_22);
    if (a == MSG_OK) {
      INFO1("Received \"%s\" ", task->name);
      INFO1("Processing \"%s\" ", task->name);
      MSG_task_execute(task);
      INFO1("\"%s\" done ", task->name);
      MSG_task_destroy(task);
    } else {
      INFO0("Hey ?! What's up ? ");
      xbt_assert0(0,"Unexpected behaviour");
    }
  }
  INFO0("I'm done. See you!");
  return 0;
}

/** Receiver function
 * This function has to be assigned to a #m_process_t that has to behave as a forwarder.
   Just like #master(), it should not be called directly.

   C style arguments (argc/argv) are interpreted as a list of host
   that will accept those tasks.

   This function keeps waiting for tasks and dispathes them to its slaves.
  */
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
	INFO1("Unknown host %s. Stopping Now! ", argv[i]);
	abort();
      }
    }
  }

  i=0;
  while(1) {
    m_task_t task = NULL;
    int a;
    a = MSG_task_get(&(task), PORT_22);
    if (a == MSG_OK) {
      INFO1("Received \"%s\" ", task->name);
      INFO2("Sending \"%s\" to \"%s\"",
		    task->name,
		    slaves[i % slaves_count]->name);
      MSG_task_put(task, slaves[i % slaves_count],
		   PORT_22);
    } else {
      INFO0("Hey ?! What's up ? ");
      xbt_assert0(0,"Unexpected behaviour");
    }
  }

  INFO0("I'm done. See you!");
  return 0;
}


/** Test function
 * This function is the core of the simulation and is divided only into 3 parts
 * thanks to MSG_create_environment() and MSG_launch_application().
 *      -# Simulation settings : MSG_create_environment() creates a realistic 
 *         environment
 *      -# Application deployment : create the agents on the right locations with  
 *         MSG_launch_application()
 *      -# The simulation is run with #MSG_main()
 * @param platform_file the name of a file containing an valid surfxml platform 
 *        description.
 * @param application_file the name of a file containing a valid surfxml application 
 *        description
 */
void test_all(const char *platform_file,const char *application_file)
{
  {				/*  Simulation setting */
    MSG_set_channel_number(MAX_CHANNEL);
    MSG_paje_output("msg_test.trace");
    MSG_create_environment(platform_file);
  }
  {                            /*   Application deployment */
    MSG_function_register("master", master);
    MSG_function_register("slave", slave);
    MSG_function_register("forwarder", forwarder);
    MSG_launch_application(application_file);
  }
  MSG_main();
  
  INFO1("Simulation time %g",MSG_getClock());
}


/** Main function
 * This initializes MSG, runs a simulation, and free all data-structures created 
 * by MSG.
 */
int main(int argc, char *argv[])
{
  MSG_global_init_args(&argc,argv);
  if (argc < 3) {
     printf ("Usage: %s platform_file deployment_file\n",argv[0]);
     printf ("example: %s msg_platform.xml msg_deployment.xml\n",argv[0]);
     exit(1);
  }
  test_all(argv[1],argv[2]);
  MSG_clean();
  return (0);
}
