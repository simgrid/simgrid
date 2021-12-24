/* simple test trying to load a DOT file.                                   */

/* Copyright (c) 2010-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/simdag.h"
#include "xbt/log.h"
#include <stdio.h>
#include <libgen.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Logging specific to this SimDag example");

int main(int argc, char **argv)
{
  xbt_dynar_t dot;

  /* initialization of SD */
  SD_init(&argc, argv);

  /* Check our arguments */
  xbt_assert(argc > 2, "Usage: %s platform_file dot_file [trace_file]"
             "example: %s ../2clusters.xml dag.dot dag.mytrace", argv[0], argv[0]);

  /* creation of the environment */
  SD_create_environment(argv[1]);

  /* load the DOT file */
  dot = SD_dotload(argv[2]);
  if(dot == NULL){
    XBT_CRITICAL("No dot loaded. Do you have a cycle in your graph?");
    exit(2);
  }

  return 0;
}
