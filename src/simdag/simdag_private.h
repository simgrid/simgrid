/* Copyright (c) 2006-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMDAG_PRIVATE_H
#define SIMDAG_PRIVATE_H
#include <set>
#include <vector>
#include "xbt/dynar.h"
#include "simgrid/simdag.h"
#include "surf/surf.h"
#include "xbt/mallocator.h"
#include <stdbool.h>
#if HAVE_JEDULE
#include "simgrid/jedule/jedule_sd_binding.h"
#endif

SG_BEGIN_DECL()

/* Global variables */

typedef struct SD_global {
  bool watch_point_reached;      /* has a task just reached a watch point? */

  std::set<SD_task_t> *initial_tasks;
  std::set<SD_task_t> *runnable_tasks;
  std::set<SD_task_t> *completed_tasks;

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
  double start_time;
  double finish_time;
  surf_action_t surf_action;
  unsigned short watch_points;  /* bit field xor()ed with masks */

  int marked;                   /* used to check if the task DAG has some cycle*/

  /* dependencies */
  std::set<SD_task_t> *inputs;
  std::set<SD_task_t> *outputs;
  std::set<SD_task_t> *predecessors;
  std::set<SD_task_t> *successors;

  /* scheduling parameters (only exist in state SD_SCHEDULED) */
  std::vector<sg_host_t> *allocation;
  double *flops_amount;
  double *bytes_amount;
  double rate;
} s_SD_task_t;

/* SimDag private functions */
XBT_PRIVATE void SD_task_set_state(SD_task_t task, e_SD_task_state_t new_state);
XBT_PRIVATE void SD_task_run(SD_task_t task);
XBT_PRIVATE bool acyclic_graph_detail(xbt_dynar_t dag);
XBT_PRIVATE void uniq_transfer_task_name(SD_task_t task);

SG_END_DECL()
#endif
