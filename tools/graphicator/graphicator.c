/* Copyright (c) 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef _XBT_WIN32
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>


#include "simdag/simdag.h"
#include "xbt/log.h"
#include "xbt/dict.h"
#include "xbt/ex.h"
#include "xbt/graph.h"
#include "surf/surf.h"
#include "surf/surf_private.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(graphicator,
                             "Graphicator Logging System");

int main(int argc, char **argv)
{
  char *platformFile = NULL;
  char *graphvizFile = NULL;

  xbt_ex_t e;

  MSG_global_init(&argc, argv);

  if (argc < 3){
    XBT_INFO("Usage: %s <platform_file.xml> <graphviz_file.dot>", argv[0]);
    return 1;
  }
  platformFile = argv[1];
  graphvizFile = argv[2];

  TRY {
    MSG_create_environment(platformFile);
  } CATCH(e) {
    xbt_die("Error while loading %s: %s",platformFile,e.msg);
  }

  //creating the graph structure
  xbt_graph_t graph = TRACE_platform_graph();
  if (graph == NULL){
    XBT_INFO ("%s expects --cfg=tracing:1", argv[0]);
  }else{
    TRACE_platform_graph_export_graphviz (graph, graphvizFile);
    XBT_INFO ("Output is in file %s", graphvizFile);
  }
  MSG_clean();
  return 0;
}
