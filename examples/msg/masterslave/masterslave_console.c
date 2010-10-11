/* Copyright (c) 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "msg/msg.h"            /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "surf/surfxml_parse.h" /* to override surf_parse and bypass the parser */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test,
                             "Messages specific for this msg example");
#define FINALIZE ((void*)221297)        /* a magic number to tell people to stop working */

int master(int argc, char *argv[]);
int slave(int argc, char *argv[]);
MSG_error_t test_all(const char *);

typedef enum {
  PORT_22 = 0,
  MAX_CHANNEL
} channel_t;

/** Emitter function  */
int master(int argc, char *argv[])
{
  long number_of_tasks = atol(argv[1]);
  double task_comp_size = atof(argv[2]);
  double task_comm_size = atof(argv[3]);
  long slaves_count = atol(argv[4]);
  int i;

  INFO2("Got %ld slaves and %ld tasks to process", slaves_count,
        number_of_tasks);
  for (i = 0; i < number_of_tasks; i++) {
    char mailbox[256];
    char sprintf_buffer[256];
    m_task_t task = NULL;

    sprintf(mailbox, "slave-%ld", i % slaves_count);
    sprintf(sprintf_buffer, "Task_%d", i);
    task =
        MSG_task_create(sprintf_buffer, task_comp_size, task_comm_size,
                        NULL);
    if (number_of_tasks < 10000 || i % 10000 == 0)
      INFO3("Sending \"%s\" (of %ld) to mailbox \"%s\"", task->name,
            number_of_tasks, mailbox);
    MSG_task_send(task, mailbox);
  }

  INFO0
      ("All tasks have been dispatched. Let's tell everybody the computation is over.");
  for (i = 0; i < slaves_count; i++) {
    char mailbox[80];

    sprintf(mailbox, "slave-%ld", i % slaves_count);
    m_task_t finalize = MSG_task_create("finalize", 0, 0, 0);
    MSG_task_send(finalize, mailbox);
  }

  INFO0("Goodbye now!");
  return 0;
}                               /* end_of_master */

/** Receiver function  */
int slave(int argc, char *argv[])
{
  m_task_t task = NULL;
  int res;
  int id = -1;
  char mailbox[80];

  xbt_assert1(sscanf(argv[1], "%d", &id),
              "Invalid argument %s\n", argv[1]);

  sprintf(mailbox, "slave-%d", id);

  while (1) {
    res = MSG_task_receive(&(task), mailbox);
    xbt_assert0(res == MSG_OK, "MSG_task_get failed");

    INFO1("Received \"%s\"", MSG_task_get_name(task));
    if (!strcmp(MSG_task_get_name(task), "finalize")) {
      MSG_task_destroy(task);
      break;
    }

    INFO1("Processing \"%s\"", MSG_task_get_name(task));
    MSG_task_execute(task);
    INFO1("\"%s\" done", MSG_task_get_name(task));
    MSG_task_destroy(task);
    task = NULL;
  }
  INFO0("I'm done. See you!");
  return 0;
}                               /* end_of_slave */

/** Test function */
MSG_error_t test_all(const char *file)  //(void)
{
  MSG_error_t res = MSG_OK;
  /*  Simulation setting */
  MSG_set_channel_number(MAX_CHANNEL);
  /*start by registering functions before loading script */
  MSG_function_register("master", master);
  MSG_function_register("slave", slave);
  MSG_load_platform_script(file);

  res = MSG_main();

  INFO1("Simulation time %g", MSG_get_clock());
  return res;
}                               /* end_of_test_all */

/** Main function */
int main(int argc, char *argv[])
{
  MSG_error_t res = MSG_OK;

  MSG_global_init(&argc, argv);
  if (argc < 2) {
    printf("Usage: %s platform_script[.lua]\n", argv[0]);
    printf("example: %s platform_script.lua\n", argv[0]);
    exit(1);
  }
  res = test_all(argv[1]);
  MSG_clean();

  if (res == MSG_OK)
    return 0;
  else
    return 1;
}                               /* end_of_main */
