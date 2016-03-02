/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

#define task_comp_size 50000000
#define task_comm_size 1000000

long number_of_jobs;
long number_of_slaves;

static long my_random(long n)
{
  return n * (rand() / ((double)RAND_MAX + 1));
}

static int master(int argc, char *argv[])
{
  int i;

  for (i = 1; i <= number_of_jobs; i++) {
    char mailbox[256];
    char sprintf_buffer[256];
    msg_task_t task = NULL;

    sprintf(mailbox, "slave-%ld", i % number_of_slaves);
    sprintf(sprintf_buffer, "Task_%d", i);
    task = MSG_task_create(sprintf_buffer, task_comp_size, task_comm_size, NULL);
    XBT_DEBUG("Sending \"%s\" (of %ld) to mailbox \"%s\"", task->name, number_of_jobs, mailbox);

    MSG_task_send(task, mailbox);
  }

  XBT_DEBUG("All tasks have been dispatched. Let's tell everybody the computation is over.");
  for (i = 0; i < number_of_slaves; i++) {
    char mailbox[80];

    sprintf(mailbox, "slave-%ld", i % number_of_slaves);
    msg_task_t finalize = MSG_task_create("finalize", 0, 0, 0);
    MSG_task_send(finalize, mailbox);
  }

  XBT_DEBUG("Goodbye now!");
  return 0;
}

static int slave(int argc, char *argv[])
{
  msg_task_t task = NULL;
  XBT_ATTRIB_UNUSED int res;

  XBT_DEBUG("mailbox: %s",MSG_process_get_name(MSG_process_self()));
  while (1) {
    res = MSG_task_receive(&(task), MSG_process_get_name(MSG_process_self()));
    xbt_assert(res == MSG_OK, "MSG_task_get failed");

    XBT_DEBUG("Received \"%s\"", MSG_task_get_name(task));
    if (!strcmp(MSG_task_get_name(task), "finalize")) {
      MSG_task_destroy(task);
      break;
    }
    XBT_DEBUG("Processing \"%s\"", MSG_task_get_name(task));
    MSG_task_execute(task);
    XBT_DEBUG("\"%s\" done", MSG_task_get_name(task));
    MSG_task_destroy(task);
    task = NULL;
  }
  return 0;
}

int main(int argc, char *argv[])
{
  msg_error_t res = MSG_OK;
  long i;

  MSG_init(&argc, argv);
  xbt_assert(argc > 3, "Usage: %s platform_file number_of_jobs number_of_slaves\n"
             "\tExample: %s msg_platform.xml 10 5\n", argv[0], argv[0]);

  MSG_function_register("master", master);
  MSG_function_register("slave", slave);

  MSG_create_environment(argv[1]);

  number_of_jobs = xbt_str_parse_int(argv[2], "Invalid amount of jobs: %s");
  number_of_slaves = xbt_str_parse_int(argv[3], "Invalid amount of slaves: %s");
  xbt_dynar_t host_dynar = MSG_hosts_as_dynar();
  long number_max = xbt_dynar_length(host_dynar);
  XBT_INFO("Got %ld slaves, %ld tasks to process, and %ld hosts", number_of_slaves, number_of_jobs,number_max);

  msg_host_t *host_table =  xbt_dynar_to_array(host_dynar);

  MSG_process_create("master", master, NULL, host_table[my_random(number_max)]);

  for(i = 0 ; i<number_of_slaves; i++) {
    char* name_host = bprintf("slave-%ld",i);
      MSG_process_create( name_host, slave, NULL, host_table[my_random(number_max)]);
      free(name_host);
  }
  xbt_free(host_table);

  res = MSG_main();

  XBT_INFO("Simulation time %g", MSG_get_clock());

  return res != MSG_OK;
}
