/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <simgrid/simdag.h>
#include <xbt/log.h>
#include <xbt/ex.h>
#include <signal.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(test,"Logging for the current example");

static int name_compare_hosts(const void *n1, const void *n2)
{
  return strcmp(
      sg_host_get_name(*(sg_host_t *) n1),
      sg_host_get_name(*(sg_host_t *) n2)
  );
}

static void scheduleDAX(xbt_dynar_t dax)
{
  unsigned int cursor;
  SD_task_t task;

  const sg_host_t *ws_list = sg_host_list();
  int totalHosts = sg_host_count();
  qsort((void *) ws_list, totalHosts, sizeof(sg_host_t), name_compare_hosts);


  xbt_dynar_foreach(dax, cursor, task) {
    if (SD_task_get_kind(task) == SD_TASK_COMP_SEQ) {
      if (!strcmp(SD_task_get_name(task), "end")
          || !strcmp(SD_task_get_name(task), "root")) {
        XBT_INFO("Scheduling %s to node: %s", SD_task_get_name(task),
                sg_host_get_name(ws_list[0]));
        SD_task_schedulel(task, 1, ws_list[0]);
      } else {
        XBT_INFO("Scheduling %s to node: %s", SD_task_get_name(task),
                sg_host_get_name(ws_list[(cursor) % totalHosts]));
        SD_task_schedulel(task, 1, ws_list[(cursor) % totalHosts]);
      }
    }
  }
}

static xbt_dynar_t initDynamicThrottling(int argc, char *argv[])
{
  SD_init(&argc, argv);
  xbt_assert(argc == 3,
      "Error on parameters.\n"
      "Usage: %s <XML platform file>  <DAX file>\n", argv[0]);

  FILE *testFile = fopen(argv[1], "r");
  xbt_assert(testFile, "Cannot open platform file %s.\n", argv[1]);
  testFile = fopen(argv[2], "r");
  xbt_assert(testFile, "Cannot open DAX file %s.\n", argv[2]);

  SD_create_environment(argv[1]);
  xbt_dynar_t dax = SD_daxload(argv[2]);

  // Schedule DAX
  XBT_INFO("Scheduling DAX...");
  scheduleDAX(dax);
  XBT_INFO("DAX scheduled");
  SD_simulate(-1);
  XBT_INFO("Simulation done.");

  return dax;
}

int main(int argc, char *argv[])
{

  /* Start process... */
  xbt_dynar_t dax = initDynamicThrottling(argc, argv);

  // Free memory
  while (!xbt_dynar_is_empty(dax)) {
    SD_task_t task = xbt_dynar_pop_as(dax, SD_task_t);
    SD_task_destroy(task);
  }
  xbt_dynar_free(&dax);
  SD_exit();
  return 0;
}
