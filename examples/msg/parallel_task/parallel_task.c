/* 	$Id$	 */

/* Copyright (c) 2002,2003,2004 Arnaud Legrand. All rights reserved.        */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "msg/msg.h"            /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "xbt/sysdep.h"         /* calloc, printf */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");

int test(int argc, char *argv[]);
MSG_error_t test_all(const char *platform_file);

/** Emitter function  */
int test(int argc, char *argv[])
{
  int slaves_count = 0;
  m_host_t *slaves = NULL;
  double task_comp_size = 100000;
  double task_comm_size = 10000;
  double *computation_amount = NULL;
  double *communication_amount = NULL;
  m_task_t ptask = NULL;
  int i, j;

  slaves_count = MSG_get_host_number();
  slaves = MSG_get_host_table();

  computation_amount = xbt_new0(double, slaves_count);
  communication_amount = xbt_new0(double, slaves_count * slaves_count);

  for (i = 0; i < slaves_count; i++)
    computation_amount[i] = task_comp_size;

  for (i = 0; i < slaves_count; i++)
    for (j = i + 1; j < slaves_count; j++)
      communication_amount[i * slaves_count + j] = task_comm_size;

  ptask = MSG_parallel_task_create("parallel task",
                                   slaves_count, slaves,
                                   computation_amount,
                                   communication_amount, NULL);
  MSG_parallel_task_execute(ptask);

  /* There is no need to free that! */
/*   free(communication_amount); */
/*   free(computation_amount); */

  INFO0("Goodbye now!");
  free(slaves);
  return 0;
}

/** Test function */
MSG_error_t test_all(const char *platform_file)
{
  MSG_error_t res = MSG_OK;

  MSG_config("workstation_model", "ptask_L07");
  MSG_set_channel_number(1);
  MSG_create_environment(platform_file);

  MSG_process_create("test", test, NULL, MSG_get_host_table()[0]);
  res = MSG_main();

  INFO1("Simulation time %g", MSG_get_clock());
  return res;
}

int main(int argc, char *argv[])
{
  MSG_error_t res = MSG_OK;

  MSG_global_init(&argc, argv);
  if (argc < 2) {
    printf("Usage: %s platform_file\n", argv[0]);
    printf("example: %s msg_platform.xml\n", argv[0]);
    exit(1);
  }
  res = test_all(argv[1]);
  MSG_clean();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}
