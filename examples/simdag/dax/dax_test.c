/* simple test trying to load a DAX file.                                   */

/* Copyright (c) 2009 Da SimGrid Team. All rights reserved.                 */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include "simdag/simdag.h"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(test,
                             "Logging specific to this SimDag example");


int main(int argc, char **argv) {
  xbt_dynar_t dax;
  unsigned int cursor;
  SD_task_t task;

  /* initialisation of SD */
  SD_init(&argc, argv);

  /* Check our arguments */
  if (argc < 3) {
    INFO1("Usage: %s platform_file dax_file", argv[0]);
    INFO1("example: %s ../sd_platform.xml Montage_50.xml", argv[0]);
    exit(1);
  }

  /* creation of the environment */
  SD_create_environment(argv[1]);

  /* load the DAX file */
  dax=SD_daxload(argv[2]);

  /* Display all the tasks */
  xbt_dynar_foreach(dax,cursor,task) {
    SD_task_dump(task);
  }
  /* exit */
  SD_exit();
  return 1;
}
