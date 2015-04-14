/* Copyright (c) 2010, 2012-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @addtogroup MSG_examples
 * 
 * - <b>tracing/trace_platform.c</b>: This program demonstrates how a
 * platform file is traced to a Paje trace file format using the tracing
 * mechanism of Simgrid.
 * You might want to run this program with the following parameters:
 * --cfg=tracing:yes
 * --cfg=tracing/categorized:yes
 * (See \ref tracing_tracing_options for details)
 */

#include <stdio.h>
#include "simgrid/msg.h"
#include "xbt/sysdep.h"         /* calloc, printf */

#include "xbt/log.h"
#include "xbt/asserts.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "Messages specific for this msg example");

/** Main function */
int main(int argc, char *argv[])
{
  MSG_init(&argc, argv);
  if (argc < 2) {
    printf("Usage: %s platform_file\n", argv[0]);
    exit(1);
  }

  char *platform_file = argv[1];
  MSG_create_environment(platform_file);
  MSG_main();
  return 0;
}
