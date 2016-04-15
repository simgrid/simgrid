/* Copyright (c) 2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"

/** @addtogroup MSG_examples
 * 
 * @section MSG_ex_tracing Tracing and visualization features
 * Tracing can be activated by various configuration options which are illustrated in these example.
 * See \ref tracing_tracing_options for details.
 * - <b>Basic example: trace-simple/trace-simple.c</b>. In this very simple program, each process creates, executes,
 *   and destroy a task. You might want to run it with the <i>--cfg=tracing/uncategorized:yes</i> option.
 */

static int simple_func(int argc, char *argv[])
{
  msg_task_t task = MSG_task_create("task", 100, 0, NULL);
  MSG_task_execute (task);
  MSG_task_destroy (task);
  return 0;
}

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);

  MSG_create_environment(argv[1]);
  MSG_process_create("simple_func", simple_func, NULL, MSG_get_host_by_name("Tremblay"));
  MSG_process_create("simple_func", simple_func, NULL, MSG_get_host_by_name("Fafard"));

  MSG_main();
  return 0;
}
