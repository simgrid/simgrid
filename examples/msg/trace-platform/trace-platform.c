/* Copyright (c) 2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @addtogroup MSG_examples
 * 
 * - <b>tracing/trace_platform.c</b>: This program demonstrates how a platform file is traced to a Paje trace file
 * format using the tracing mechanism of Simgrid. You might want to run this program with the following parameters:
 * --cfg=tracing:yes
 * --cfg=tracing/categorized:yes
 * (See \ref tracing_tracing_options for details)
 */

#include "simgrid/msg.h"

int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  xbt_assert(argc > 1, "Usage: %s platform_file\n\tExample: %s msg_platform.xml\n", argv[0], argv[0]);
  MSG_create_environment(argv[1]);
  MSG_main();
  return 0;
}
