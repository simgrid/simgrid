/* Copyright (c) 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMDAG_PRIVATE_H
#define SIMDAG_PRIVATE_H

#include "xbt/dict.h"
#include "xbt/dynar.h"
#include "xbt/fifo.h"
#include "simdag/simdag.h"
#include "simdag/datatypes.h"
#include "surf/surf.h"

#define SD_INITIALISED() (sd_global != NULL)
#define SD_CHECK_INIT_DONE() xbt_assert0(SD_INITIALISED(), "Call SD_init() first");

/* Global variables */

typedef struct SD_global {
  xbt_dict_t workstations;      /* workstation dictionary */
  int workstation_count;        /* number of workstations */
  SD_workstation_t *workstation_list;   /* array of workstations, created only if
                                           necessary in SD_workstation_get_list */

  xbt_dict_t links;             /* links */
  int link_count;               /* number of links */
  SD_link_t *link_list;         /* array of links, created only if
                                   necessary in SD_link_get_list */
  SD_link_t *recyclable_route;  /* array returned by SD_route_get_list
                                   and mallocated only once */

  int watch_point_reached;      /* has a task just reached a watch point? */

  /* task state sets */
  xbt_swag_t not_scheduled_task_set;
  xbt_swag_t ready_task_set;
  xbt_swag_t scheduled_task_set;
  xbt_swag_t runnable_task_set;
  xbt_swag_t in_fifo_task_set;
  xbt_swag_t running_task_set;
  xbt_swag_t done_task_set;
  xbt_swag_t failed_task_set;

  int task_number;

} s_SD_global_t, *SD_global_t;

extern SD_global_t sd_global;

/* Link */
typedef struct SD_link {
  void *surf_link;              /* surf object */
  void *data;                   /* user data */
  e_SD_link_sharing_policy_t sharing_policy;
} s_SD_link_t;

/* Workstation */
typedef struct SD_workstation {
  void *surf_workstation;       /* surf object */
  void *data;                   /* user data */
  e_SD_workstation_access_mode_t access_mode;

  xbt_fifo_t task_fifo;         /* only used in sequential mode */
  SD_task_t current_task;       /* only used in sequential mode */
} s_SD_workstation_t;

/* Task */
typedef struct SD_task {
  s_xbt_swag_hookup_t state_hookup;
  xbt_swag_t state_set;
  e_SD_task_state_t state;
  void *data;                   /* user data */
  char *name;
  int kind;
  double amount;
  double remains;
  double start_time;
  double finish_time;
  surf_action_t surf_action;
  unsigned short watch_points; /* bit field xor()ed with masks */

  int fifo_checked;             /* used by SD_task_just_done to make sure we evaluate
                                   the task only once */

  /* dependencies */
  xbt_dynar_t tasks_before;
  xbt_dynar_t tasks_after;
  unsigned int unsatisfied_dependencies;

  /* scheduling parameters (only exist in state SD_SCHEDULED) */
  int workstation_nb;
  SD_workstation_t *workstation_list;   /* surf workstations */
  double *computation_amount;
  double *communication_amount;
  double rate;
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

SD_link_t __SD_link_create(void *surf_link, void *data);
void __SD_link_destroy(void *link);

SD_workstation_t __SD_workstation_create(void *surf_workstation, void *data);
void __SD_workstation_destroy(void *workstation);
int __SD_workstation_is_busy(SD_workstation_t workstation);

void __SD_task_set_state(SD_task_t task, e_SD_task_state_t new_state);
void __SD_task_really_run(SD_task_t task);
int __SD_task_try_to_run(SD_task_t task);
void __SD_task_just_done(SD_task_t task);

/* Functions to test if the task is in a given state. */

/* Returns whether the given task is scheduled or runnable. */
static XBT_INLINE int __SD_task_is_scheduled_or_runnable(SD_task_t task)
{
  return task->state_set == sd_global->scheduled_task_set ||
    task->state_set == sd_global->runnable_task_set;
}

/* Returns whether the state of the given task is SD_NOT_SCHEDULED. */
static XBT_INLINE int __SD_task_is_not_scheduled(SD_task_t task)
{
  return task->state_set == sd_global->not_scheduled_task_set;
}

/* Returns whether the state of the given task is SD_SCHEDULED. */
static XBT_INLINE int __SD_task_is_ready(SD_task_t task)
{
  return task->state_set == sd_global->ready_task_set;
}

/* Returns whether the state of the given task is SD_SCHEDULED. */
static XBT_INLINE int __SD_task_is_scheduled(SD_task_t task)
{
  return task->state_set == sd_global->scheduled_task_set;
}

/* Returns whether the state of the given task is SD_READY. */
static XBT_INLINE int __SD_task_is_runnable(SD_task_t task)
{
  return task->state_set == sd_global->runnable_task_set;
}

/* Returns whether the state of the given task is SD_IN_FIFO. */
static XBT_INLINE int __SD_task_is_in_fifo(SD_task_t task)
{
  return task->state_set == sd_global->in_fifo_task_set;
}

/* Returns whether the state of the given task is SD_READY or SD_IN_FIFO. */
static XBT_INLINE int __SD_task_is_runnable_or_in_fifo(SD_task_t task)
{
  return task->state_set == sd_global->runnable_task_set ||
    task->state_set == sd_global->in_fifo_task_set;
}

/* Returns whether the state of the given task is SD_RUNNING. */
static XBT_INLINE int __SD_task_is_running(SD_task_t task)
{
  return task->state_set == sd_global->running_task_set;
}

#endif
