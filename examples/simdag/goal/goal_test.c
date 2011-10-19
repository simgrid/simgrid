/* GOAL loader prototype. Not ready for public usage yet */

/* Copyright (c) 2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdlib.h>
#include <stdio.h>
#include "simdag/simdag.h"
#include "xbt/log.h"
#include "xbt/ex.h"
#include <string.h>

XBT_LOG_NEW_DEFAULT_CATEGORY(goal, "The GOAL loader into SimDag");

typedef struct {
  int i, j, k;
} s_bcast_task_t,*bcast_task_t;


const SD_workstation_t* ws_list;
int count = 0;

static void send_one(int from, int to) {
  //XBT_DEBUG("send_one(%d, %d)",from,to);

  if (count %1000 == 0)
    XBT_INFO("Sending task #%d",count);
  count++;

  bcast_task_t bt = xbt_new(s_bcast_task_t,1);

  bt->i=from;
  bt->j=(from+to)/2;
  bt->k=to;

  SD_task_t t = SD_task_create_comm_e2e("Blab",bt,424242);
  XBT_DEBUG("Schedule task between %d and %d",bt->i,bt->j);
  SD_task_schedulel(t,2,ws_list[bt->i],ws_list[bt->j]);
  SD_task_watch(t,SD_DONE);
}

int main(int argc, char **argv) {

  /* initialization of SD */
  SD_init(&argc, argv);

  if (argc > 1) {
    SD_create_environment(argv[1]);
  } else {
    SD_create_environment("../../platforms/One_cluster_no_backbone.xml");
  }

  ws_list = SD_workstation_get_list();
  xbt_dynar_t done = NULL;
  send_one(0,10000);
  do {
    if (done != NULL && xbt_dynar_length(done) > 0) {
      unsigned int cursor;
      SD_task_t task;

      xbt_dynar_foreach(done, cursor, task) {
        bcast_task_t bt = SD_task_get_data(task);

        if (bt->i != bt->j -1)
          send_one(bt->i,bt->j);
        if (bt->j != bt->k -1)
          send_one(bt->j,bt->k);

        SD_task_destroy(task);
        free(bt);
      }
      xbt_dynar_free(&done);
    }
    done=SD_simulate(-1);
  } while(xbt_dynar_length(done) > 0);

  SD_exit();
  return 0;
}
