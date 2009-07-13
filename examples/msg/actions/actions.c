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

static void sleep(xbt_dynar_t action)
{
  char *name = xbt_str_join(action, " ");
  char *duration = xbt_dynar_get_as(action, 2, char *);
  INFO1("sleep: %s", name);
  MSG_process_sleep(parse_double(duration));
  INFO1("sleept: %s", name);
  free(name);
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
  MSG_action_register("send", send);
  MSG_action_register("recv", recv);
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
