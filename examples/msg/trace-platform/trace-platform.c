/* Copyright (c) 2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** @addtogroup MSG_examples
 * 
 * - <b>Tracing the platform: trace-platform/trace-platform.c</b>. This example just loads a platform file to
 *  demonstrate how it can be traced into the Paje trace file format. You might want to run this program with the
 *  following options: <i>--cfg=tracing:yes</i> and <i>--cfg=tracing/categorized:yes</i>.
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
