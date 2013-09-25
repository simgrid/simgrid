/* simple test trying to load a DOT file.                                   */

/* Copyright (c) 2010-2012. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <stdio.h>
#include "simdag/simdag.h"
#include "xbt/log.h"
#include "xbt/ex.h"
#include <string.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(test,
                             "Logging specific to this SimDag example");

int main(int argc, char **argv)
{
  xbt_dynar_t dot;
  unsigned int cursor;
  SD_task_t task, *dot_as_array=NULL;

  /* initialisation of SD */
  SD_init(&argc, argv);

  /* Check our arguments */
  if (argc < 2) {
    XBT_INFO("Usage: %s dot_file", argv[0]);
    exit(1);
  }

  /* load the DOT file */
  dot = SD_dotload(argv[1]);

  /* Display all the tasks */
  XBT_INFO
      ("------------------- Display all tasks of the loaded DAG ---------------------------");
  xbt_dynar_foreach(dot, cursor, task) {
      SD_task_dump(task);
    }

  XBT_INFO
      ("--------------------- Transform the dynar into an array ---------------------------");
  cursor=0;
  dot_as_array = (SD_task_t*) xbt_dynar_to_array(dot);
  XBT_INFO
      ("----------------------------- dump tasks again ------------------------------------");
  while ((task=dot_as_array[cursor++])){
    SD_task_dump(task);
  }

  cursor=0;
  while ((task=dot_as_array[cursor++])){
    SD_task_destroy(task);
  }

  free(dot_as_array);

  /* exit */
  SD_exit();
  return 0;
}
