/* Copyright (c) 2008-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/msg.h"
#include "xbt/graph.h"

int main(int argc, char **argv)
{
  MSG_init(&argc, argv);

  xbt_assert(argc == 3, "Usage: %s <platform_file.xml> <graphviz_file.dot>", argv[0]);

  MSG_create_environment(argv[1]);
  int status = TRACE_platform_graph_export_graphviz (argv[2]);

  xbt_assert(status != 0, "%s expects --cfg=tracing:yes --cfg=tracing/platform:yes", argv[0]);
  MSG_main(); /* useless, except for correctly cleaning memory */
  return 0;
}
