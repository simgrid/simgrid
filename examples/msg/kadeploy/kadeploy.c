/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * Copyright (c) 2012. Maximiliano Geier.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include<stdio.h>

#include "msg/msg.h"            /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "xbt/sysdep.h"         /* calloc */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"

/** @addtogroup MSG_examples
 * 
 *  - <b>kadeploy/kadeploy.c: Kadeploy implementation</b>.
 */


XBT_LOG_NEW_DEFAULT_CATEGORY(msg_kadeploy,
                             "Messages specific for kadeploy");

/*
 Data structures
 */

/* Random iterator for xbt_dynar */
typedef struct xbt_dynar_iterator_struct {
  xbt_dynar_t list;
  xbt_dynar_t indices_list;
  unsigned int current;
  unsigned long length;
  unsigned int (*criteria_fn)(void* it);
} *xbt_dynar_iterator_t;
typedef struct xbt_dynar_iterator_struct xbt_dynar_iterator_s;


xbt_dynar_iterator_t xbt_dynar_iterator_new(xbt_dynar_t list, unsigned int (*criteria_fn)(void*));
void *xbt_dynar_iterator_next(xbt_dynar_iterator_t it);
void xbt_dynar_iterator_delete(xbt_dynar_iterator_t it);
unsigned int xbt_dynar_iterator_forward_criteria(void *p);

int broadcaster(int argc, char *argv[]);
int peer(int argc, char *argv[]);

void check_hosts(const int count, char **list);
xbt_dynar_t build_hostlist_from_argv(int argc, char *argv[]);
void build_chain(xbt_dynar_t host_list);

int peer_wait_for_init();

msg_error_t test_all(const char *platform_file,
                     const char *application_file);

double task_comm_size_lat = 10e0;
double task_comm_size_bw = 10e8;

/* Allocates and initializes a new xbt_dynar iterator for list, using criteria_fn as iteration criteria
   criteria_fn: given an iterator, it must update the iterator and give the next element's index, 
   less than 0 otherwise*/
xbt_dynar_iterator_t xbt_dynar_iterator_new(xbt_dynar_t list, unsigned int (*criteria_fn)(void*))
{
  xbt_dynar_iterator_t it = xbt_new(xbt_dynar_iterator_s, 1);
  
  it->list = list;
  it->length = xbt_dynar_length(list);
  it->indices_list = xbt_dynar_new(sizeof(unsigned int), NULL);
  it->criteria_fn = criteria_fn;
  it->current = -1;
}

/* Returns the next element iterated by iterator it, NULL if there are no more elements */
void *xbt_dynar_iterator_next(xbt_dynar_iterator_t it)
{
  unsigned int next = it->criteria_fn((xbt_dynar_iterator_t)it);
  XBT_INFO("%d current\n", next);
  if (next < 0)
    return NULL;
  else {
    xbt_dynar_push(it->indices_list, &next);
    return xbt_dynar_get_ptr(it->list, next);
  }
}

void xbt_dynar_iterator_delete(xbt_dynar_iterator_t it)
{
  xbt_dynar_free_container(&(it->indices_list));
  xbt_free_ref(&it);
}

unsigned int xbt_dynar_iterator_forward_criteria(void *p)
{
  xbt_dynar_iterator_t it = (xbt_dynar_iterator_t)p;
  unsigned int r = -1;
  if (it->current == -1) {
    /* iterator initialization */
    it->current = 0;
  }
  if (it->current < it->length) {
    r = it->current;
    it->current++;
  }

  return r;
}

xbt_dynar_t build_hostlist_from_argv(int argc, char *argv[])
{
  xbt_dynar_t host_list = xbt_dynar_new(sizeof(char*), NULL);
  msg_host_t h = NULL;
  int i = 1;
  
  for (; i < argc; i++) {
    XBT_INFO("host%d = %s", i, argv[i]);
    h = MSG_get_host_by_name(argv[i]);
    if (h == NULL) {
      XBT_INFO("Unknown host %s. Stopping Now! ", argv[i]);
      abort();
    } else {
      xbt_dynar_push(host_list, &(argv[i]));
    }
  }
  return host_list;
}

void delete_hostlist(xbt_dynar_t h)
{
  xbt_dynar_free_container(&h);
}

void build_chain(xbt_dynar_t host_list)
{
  xbt_dynar_iterator_t it = xbt_dynar_iterator_new(host_list, xbt_dynar_iterator_forward_criteria);
  char **cur = NULL;

  for (cur = (char**)xbt_dynar_iterator_next(it); cur != NULL; cur = (char**)xbt_dynar_iterator_next(it)) {
    XBT_INFO("iterating host = %s", *cur);
  }
}

/*void setup_chain_criteria(chain_criteria_t c, char *(*fn)(void))
{
  
}

void build_chain(const int hostcount, char **hostlist)
{
  int i;
  for (i = 0; i < hostcount; i++) {
    
  }
}*/

/** Emitter function  */
int broadcaster(int argc, char *argv[])
{
  double time;
  xbt_dynar_t host_list = NULL;
  msg_task_t task_la = NULL;
  msg_task_t task_bw = NULL;
  char sprintf_buffer_la[64];
  char sprintf_buffer_bw[64];

  XBT_INFO("broadcaster");

  /* Check that every host in the command line actually exists and add it to a dynamic array */
  host_list = build_hostlist_from_argv(argc, argv);
  
  build_chain(host_list);

  /* Latency */
  /*time = MSG_get_clock();
  sprintf(sprintf_buffer_la, "latency task");
  task_la =
      MSG_task_create(sprintf_buffer_la, 0.0, task_comm_size_lat, NULL);
  task_la->data = xbt_new(double, 1);
  *(double *) task_la->data = time;
  XBT_INFO("task_la->data = %le", *((double *) task_la->data));
  MSG_task_send(task_la, argv[1]);*/

  /* Bandwidth */
  /*time = MSG_get_clock();
  sprintf(sprintf_buffer_bw, "bandwidth task");
  task_bw =
      MSG_task_create(sprintf_buffer_bw, 0.0, task_comm_size_bw, NULL);
  task_bw->data = xbt_new(double, 1);
  *(double *) task_bw->data = time;
  XBT_INFO("task_bw->data = %le", *((double *) task_bw->data));
  MSG_task_send(task_bw, argv[1]);
  */
  return 0;
}                               /* end_of_client */

int peer_wait_for_init()
{
  return MSG_OK;
}

/** Peer function  */
int peer(int argc, char *argv[])
{
  double time, time1, sender_time;
  msg_task_t task_la = NULL;
  msg_task_t task_bw = NULL;
  int a;
  double communication_time = 0;

  XBT_INFO("peer");

  time = MSG_get_clock();

  a = peer_wait_for_init();
  /* Get Latency */
  /*a = MSG_task_receive(&task_la,MSG_host_get_name(MSG_host_self()));
  if (a == MSG_OK) {
    time1 = MSG_get_clock();
    sender_time = *((double *) (task_la->data));
    time = sender_time;
    communication_time = time1 - time;
    XBT_INFO("Task received : %s", task_la->name);
    xbt_free(task_la->data);
    MSG_task_destroy(task_la);
    XBT_INFO("Communic. time %le", communication_time);
    XBT_INFO("--- la %f ----", communication_time);
  } else {
    xbt_die("Unexpected behavior");
  }*/


  /* Get Bandwidth */
  /*a = MSG_task_receive(&task_bw,MSG_host_get_name(MSG_host_self()));
  if (a == MSG_OK) {
    time1 = MSG_get_clock();
    sender_time = *((double *) (task_bw->data));
    time = sender_time;
    communication_time = time1 - time;
    XBT_INFO("Task received : %s", task_bw->name);
    xbt_free(task_bw->data);
    MSG_task_destroy(task_bw);
    XBT_INFO("Communic. time %le", communication_time);
    XBT_INFO("--- bw %f ----", task_comm_size_bw / communication_time);
  } else {
    xbt_die("Unexpected behavior");
  }*/


  return 0;
}                               /* end_of_receiver */


/** Test function */
msg_error_t test_all(const char *platform_file,
                     const char *application_file)
{

  msg_error_t res = MSG_OK;



  XBT_INFO("test_all");

  /*  Simulation setting */
  MSG_create_environment(platform_file);

  /*   Application deployment */
  MSG_function_register("broadcaster", broadcaster);
  MSG_function_register("peer", peer);

  MSG_launch_application(application_file);

  res = MSG_main();

  return res;
}                               /* end_of_test_all */


/** Main function */
int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;

#ifdef _MSC_VER
  unsigned int prev_exponent_format =
      _set_output_format(_TWO_DIGIT_EXPONENT);
#endif

  MSG_init(&argc, argv);


  if (argc != 3) {
    XBT_CRITICAL("Usage: %s platform_file deployment_file <model>\n",
              argv[0]);
    XBT_CRITICAL
        ("example: %s msg_platform.xml msg_deployment.xml KCCFLN05_Vegas\n",
         argv[0]);
    exit(1);
  }

  /* Options for the workstation/model:

     KCCFLN05              => for maxmin
     KCCFLN05_proportional => for proportional (Vegas)
     KCCFLN05_Vegas        => for TCP Vegas
     KCCFLN05_Reno         => for TCP Reno
   */
  //MSG_config("workstation/model", argv[3]);

  res = test_all(argv[1], argv[2]);

  XBT_INFO("Total simulation time: %le", MSG_get_clock());

  MSG_clean();

#ifdef _MSC_VER
  _set_output_format(prev_exponent_format);
#endif

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
