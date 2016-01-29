/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMDAG_PRIVATE_H
#define SIMDAG_PRIVATE_H

#include "xbt/base.h"
#include "xbt/dynar.h"
#include "simgrid/simdag.h"
#include "surf/surf.h"
#include "xbt/mallocator.h"
#include <stdbool.h>

SG_BEGIN_DECL()

/* Global variables */

typedef struct SD_global {
  xbt_mallocator_t task_mallocator; /* to not remalloc new tasks */

  int watch_point_reached;      /* has a task just reached a watch point? */

  xbt_dynar_t initial_task_set;
  xbt_dynar_t executable_task_set;
  xbt_dynar_t completed_task_set;

  xbt_dynar_t return_set;

} s_SD_global_t, *SD_global_t;

extern XBT_PRIVATE SD_global_t sd_global;

/* Task */
typedef struct SD_task {
  e_SD_task_state_t state;
  void *data;                   /* user data */
  char *name;
  e_SD_task_kind_t kind;
  double amount;
  double alpha;          /* used by typed parallel tasks */
  double remains;
  double start_time;
  double finish_time;
  surf_action_t surf_action;
  unsigned short watch_points;  /* bit field xor()ed with masks */

  int marked;                   /* used to check if the task DAG has some cycle*/

  /* dependencies */
  xbt_dynar_t tasks_before;
  xbt_dynar_t tasks_after;
  int unsatisfied_dependencies;
  unsigned int is_not_ready;

  /* scheduling parameters (only exist in state SD_SCHEDULED) */
  int host_count;
  sg_host_t *host_list;
  double *flops_amount;
  double *bytes_amount;
  double rate;

  long long int counter;        /* task unique identifier for instrumentation */
  char *category;               /* sd task category for instrumentation */
} s_SD_task_t;

/* Task dependencies */
typedef struct SD_dependency {
  char *name;
  void *data;
  SD_task_t src;
  SD_task_t dst;
  /* src must be finished before dst can start */
} s_SD_dependency_t, *SD_dependency_t;

/* SimDag private functions */
XBT_PRIVATE void SD_task_set_state(SD_task_t task, e_SD_task_state_t new_state);
XBT_PRIVATE void SD_task_run(SD_task_t task);
XBT_PRIVATE bool acyclic_graph_detail(xbt_dynar_t dag);
XBT_PRIVATE void uniq_transfer_task_name(SD_task_t task);

/* Task mallocator functions */
XBT_PRIVATE void* SD_task_new_f(void);
XBT_PRIVATE void SD_task_recycle_f(void *t);
XBT_PRIVATE void SD_task_free_f(void *t);

SG_END_DECL()
#endif
