/* 	$Id$	 */

/* Copyright (c) 2009. The SimGrid team. All rights reserved.               */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include "msg/msg.h"            /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "xbt.h"                /* calloc, printf */


XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                 "Messages specific for this msg example");
int communicator_size=0;

typedef struct coll_ctr_t{
  int bcast_counter;
  int reduce_counter;
} *coll_ctr;

/* Helper function */
static double parse_double(const char *string) {
  double value;
  char *endptr;

  value=strtod(string, &endptr);
  if (*endptr != '\0')
	  THROW1(unknown_error, 0, "%s is not a double", string);
  return value;
}


/* My actions */
static void send(xbt_dynar_t action)
{
  char *name = xbt_str_join(action, " ");
  char *to = xbt_dynar_get_as(action, 2, char *);
  char *size = xbt_dynar_get_as(action, 3, char *);
  INFO2("Send: %s (size: %lg)", name, parse_double(size));
  MSG_task_send(MSG_task_create(name, 0, parse_double(size), NULL), to);
  INFO1("Sent %s", name);
  free(name);
}


static int spawned_send(int argc, char *argv[])
{
  INFO3("%s: Sending %s on %s", MSG_process_self()->name, 
	argv[1],argv[0]);
  MSG_task_send(MSG_task_create(argv[0], 0, parse_double(argv[1]), NULL), 
		argv[0]);
  return 0;
}

static void Isend(xbt_dynar_t action)
{
  char spawn_name[80];
  char *to = xbt_dynar_get_as(action, 2, char *);
  char *size = xbt_dynar_get_as(action, 3, char *);
  char **myargv;
  m_process_t comm_helper;

  INFO1("Isend on %s: spawn process ", 
	 MSG_process_self()->name);

  myargv = (char**) calloc (3, sizeof (char*));
  
  myargv[0] = xbt_strdup(to);
  myargv[1] = xbt_strdup(size);
  myargv[2] = NULL;

  sprintf(spawn_name,"%s_wait",MSG_process_self()->name);
  comm_helper =
    MSG_process_create_with_arguments(spawn_name, spawned_send, 
				      NULL, MSG_host_self(), 2, myargv);
}


static void recv(xbt_dynar_t action)
{
  char *name = xbt_str_join(action, " ");
  m_task_t task = NULL;
  INFO1("Receiving: %s", name);
  //FIXME: argument of action ignored so far; semantic not clear
  //char *from=xbt_dynar_get_as(action,2,char*);
  MSG_task_receive(&task, MSG_process_get_name(MSG_process_self()));
  INFO1("Received %s", MSG_task_get_name(task));
  MSG_task_destroy(task);
  free(name);
}

static int spawned_recv(int argc, char *argv[])
{
  m_task_t task = NULL;
  char* name = (char *) MSG_process_self()->data;
  INFO1("Receiving on %s", name);
  MSG_task_receive(&task, name);
  INFO1("Received %s", MSG_task_get_name(task));
  INFO1("waiter on %s", MSG_process_self()->name);
  MSG_task_send(MSG_task_create("waiter",0,0,NULL),MSG_process_self()->name); 
  
  MSG_task_destroy(task);
  return 0;
}


static void Irecv(xbt_dynar_t action)
{
  char *name = xbt_str_join(action, " ");
  m_process_t comm_helper;

  INFO1("Irecv on %s: spawn process ", 
	 MSG_process_get_name(MSG_process_self()));

  sprintf(name,"%s_wait",MSG_process_self()->name);
  comm_helper = MSG_process_create(name,spawned_recv,
                 (void *) MSG_process_self()->name,
                 MSG_host_self());


  free(name);
}


static void wait(xbt_dynar_t action)
{
  char *name = xbt_str_join(action, " ");
  char task_name[80];
  m_task_t task = NULL;
  
  INFO1("wait: %s", name);
  sprintf(task_name,"%s_wait",MSG_process_self()->name);
  MSG_task_receive(&task,task_name);
  MSG_task_destroy(task);
  INFO1("waited: %s", name);
  free(name);
}

static void barrier (xbt_dynar_t action)
{
  char *name = xbt_str_join(action, " ");
  INFO1("barrier: %s", name);
  

  free(name);

}

static void reduce(xbt_dynar_t action)
{
  int i;
  char *name;
  char task_name[80];
  char spawn_name[80];
  char *comm_size = xbt_dynar_get_as(action, 2, char *);
  char *comp_size = xbt_dynar_get_as(action, 3, char *);
  m_process_t comm_helper=NULL;
  m_task_t task=NULL, comp_task=NULL;
  const char* process_name;
  
  coll_ctr counters =  (coll_ctr) MSG_process_get_data(MSG_process_self());

  xbt_assert0(communicator_size, "Size of Communicator is not defined"
	      ", can't use collective operations");

  MSG_process_self()->data=NULL;

  process_name = MSG_process_self()->name;

  if (!counters){
    DEBUG0("Initialize the counters");
    counters = (coll_ctr) calloc (1, sizeof(struct coll_ctr_t));
  }

  name = bprintf("reduce_%d", counters->reduce_counter++);

  if (!strcmp(process_name, "process0")){
    INFO2("%s: %s is the Root",name, process_name);
    for(i=1;i<communicator_size;i++){
      sprintf(spawn_name,"%s_process%d", name, i);
      sprintf(task_name,"%s_wait", spawn_name);
      comm_helper = 
	MSG_process_create(task_name, spawned_recv, 
			   (void *) xbt_strdup(spawn_name),
			   MSG_host_self());      
    }

    for(i=1;i<communicator_size;i++){
      sprintf(task_name,"%s_process%d_wait", name, i);
      MSG_task_receive(&task,task_name);
      MSG_task_destroy(task);
      task=NULL;
    }

    comp_task = 
      MSG_task_create("reduce_comp", parse_double(comp_size), 0, NULL);
    INFO1("%s: computing 'reduce_comp'", name);
    MSG_task_execute(comp_task);
    MSG_task_destroy(comp_task);
    INFO1("%s: computed", name);
  } else {
    INFO2("%s: %s sends", name, process_name);
    sprintf(task_name,"%s_%s", name, process_name);
    INFO1("put on %s", task_name);
    MSG_task_send(MSG_task_create(name, 0, parse_double(comm_size), NULL),
		  task_name);
  }

  MSG_process_set_data(MSG_process_self(), (void*)counters);
  free(name);
}

static void bcast (xbt_dynar_t action)
{
  int i;
  char *name;
  const char* process_name;
  char task_name[80];
  char spawn_name[80];
  char **myargv;
  m_process_t comm_helper=NULL;
  m_task_t task=NULL;
  char *size = xbt_dynar_get_as(action, 2, char *);
  coll_ctr counters =  (coll_ctr) MSG_process_get_data(MSG_process_self());

  xbt_assert0(communicator_size, "Size of Communicator is not defined"
	      ", can't use collective operations");

  MSG_process_self()->data=NULL;

  process_name = MSG_process_self()->name;
  if (!counters){
    DEBUG0("Initialize the counters");
    counters = (coll_ctr) calloc (1, sizeof(struct coll_ctr_t));
  }

  name = bprintf("bcast_%d", counters->bcast_counter++);
  if (!strcmp(process_name, "process0")){
    INFO2("%s: %s is the Root",name, process_name);

    for(i=1;i<communicator_size;i++){
      myargv = (char**) calloc (3, sizeof (char*));
      myargv[0] = xbt_strdup(name);
      myargv[1] = xbt_strdup(size);
      myargv[2] = NULL;

      sprintf(spawn_name,"%s_%d", myargv[0], i);
      comm_helper = 
	MSG_process_create_with_arguments(spawn_name, spawned_send, 
					  NULL, MSG_host_self(), 2, myargv);
    }
    
    for(i=1;i<communicator_size;i++){
      sprintf(task_name,"process%d_wait", i);
      DEBUG1("get on %s", task_name);
      MSG_task_receive(&task,task_name);      
      MSG_task_destroy(task);
      task=NULL;
    }
    INFO2("%s: all messages sent by %s have been received",
	  name, process_name);
  } else {
    INFO2("%s: %s receives", name, process_name);
    MSG_task_receive(&task, name);
    MSG_task_destroy(task);
    INFO2("%s: %s has received", name,process_name);
    sprintf(task_name,"%s_wait", process_name);
    DEBUG1("put on %s", task_name);
    MSG_task_send(MSG_task_create("waiter",0,0,NULL),task_name);
  }

  MSG_process_set_data(MSG_process_self(), (void*)counters);
  free(name);
}


static void sleep(xbt_dynar_t action)
{
  char *name = xbt_str_join(action, " ");
  char *duration = xbt_dynar_get_as(action, 2, char *);
  INFO1("sleep: %s", name);
  MSG_process_sleep(parse_double(duration));
  INFO1("sleept: %s", name);
  free(name);
}

static void comm_size(xbt_dynar_t action)
{
  char *size = xbt_dynar_get_as(action, 2, char *);
  communicator_size = parse_double(size);
}

static void compute(xbt_dynar_t action)
{
  char *name = xbt_str_join(action, " ");
  char *amout = xbt_dynar_get_as(action, 2, char *);
  m_task_t task = MSG_task_create(name, parse_double(amout), 0, NULL);
  INFO1("computing: %s", name);
  MSG_task_execute(task);
  MSG_task_destroy(task);
  INFO1("computed: %s", name);
  free(name);
}

/** Main function */
int main(int argc, char *argv[])
{
  MSG_error_t res = MSG_OK;
  
  /* Check the given arguments */
  MSG_global_init(&argc, argv);
  if (argc < 4) {
    printf("Usage: %s platform_file deployment_file action_files\n", argv[0]);
    printf("example: %s msg_platform.xml msg_deployment.xml actions\n",
           argv[0]);
    exit(1);
  }

  /*  Simulation setting */
  MSG_create_environment(argv[1]);

  /* No need to register functions as in classical MSG programs: the actions get started anyway */
  MSG_launch_application(argv[2]);

  /*   Action registration */
  MSG_action_register("comm_size", comm_size);
  MSG_action_register("send", send);
  MSG_action_register("Isend", Isend);
  MSG_action_register("recv", recv);
  MSG_action_register("Irecv", Irecv);
  MSG_action_register("wait", wait);
  MSG_action_register("barrier", barrier);
  MSG_action_register("bcast", bcast);
  MSG_action_register("reduce", reduce);
  MSG_action_register("sleep", sleep);
  MSG_action_register("compute", compute);


  /* Actually do the simulation using MSG_action_trace_run */
  res = MSG_action_trace_run(argv[3]);

  INFO1("Simulation time %g", MSG_get_clock());
  MSG_clean();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
