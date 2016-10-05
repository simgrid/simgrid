/* Example of scatter communication, accepting a large amount of processes. 
 * This based the experiment of Fig. 4 in http://hal.inria.fr/hal-00650233/
 * That experiment is a comparison to the LogOPSim simulator, that takes 
 * GOAL files as an input, thus the file name. But there is no actual link
 * to the GOAL formalism beside of this.
 */

/* Copyright (c) 2011-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include "simgrid/simdag.h"
#include "xbt/xbt_os_time.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(goal, "The GOAL loader into SimDag");

typedef struct {
  int i, j, k;
} s_bcast_task_t,*bcast_task_t;

const sg_host_t* ws_list;
int count = 0;

xbt_dynar_t reclaimed;

static void send_one(int from, int to) {
  //XBT_DEBUG("send_one(%d, %d)",from,to);

  if (count %100000 == 0)
    XBT_INFO("Sending task #%d",count);
  count++;

  bcast_task_t bt;
  if (!xbt_dynar_is_empty(reclaimed)) {
     bt = xbt_dynar_pop_as(reclaimed,bcast_task_t);
  } else {
    bt = xbt_new(s_bcast_task_t,1);
  }
  bt->i=from;
  bt->j=(from+to)/2;
  bt->k=to;

  SD_task_t task = SD_task_create_comm_e2e(NULL,bt,424242);

  XBT_DEBUG("Schedule task between %d and %d",bt->i,bt->j);
  SD_task_schedulel(task,2,ws_list[bt->i],ws_list[bt->j]);
  SD_task_watch(task,SD_DONE);
}

int main(int argc, char **argv) {
  xbt_os_timer_t timer = xbt_os_timer_new();

  /* initialization of SD */
  SD_init(&argc, argv);

  if (argc > 1) {
    SD_create_environment(argv[1]);
  } else {
    SD_create_environment("../../platforms/One_cluster_no_backbone.xml");
  }

  ws_list = sg_host_list();
  reclaimed = xbt_dynar_new(sizeof(bcast_task_t),xbt_free_ref);
  xbt_dynar_t done = NULL;

  xbt_os_cputimer_start(timer);
  send_one(0,sg_host_count());
  do {
    if (done != NULL && !xbt_dynar_is_empty(done)) {
      unsigned int cursor;
      SD_task_t task;

      xbt_dynar_foreach(done, cursor, task) {
        bcast_task_t bt = SD_task_get_data(task);

        if (bt->i != bt->j -1)
          send_one(bt->i,bt->j);
        if (bt->j != bt->k -1)
          send_one(bt->j,bt->k);

        if (xbt_dynar_length(reclaimed)<100) {
          xbt_dynar_push_as(reclaimed,bcast_task_t,bt);
        } else {
          free(bt);
        }
        SD_task_destroy(task);
      }
      xbt_dynar_free(&done);
    }
    done=SD_simulate(-1);
  } while(!xbt_dynar_is_empty(done));
  xbt_os_cputimer_stop(timer);
  printf("exec_time:%f\n", xbt_os_timer_elapsed(timer) );

  xbt_dynar_free(&done);
  xbt_dynar_free(&reclaimed);

  SD_exit();
  XBT_INFO("Done. Bailing out");
  return 0;
}
