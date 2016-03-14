/* Copyright (c) 2009-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* simple test to schedule a DAX file with the Min-Min algorithm.           */
#include <string.h>
#include "simgrid/simdag.h"

#if HAVE_JEDULE
#include "simgrid/jedule/jedule_sd_binding.h"
#endif

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Logging specific to this SimDag example");

typedef struct _HostAttribute *HostAttribute;
struct _HostAttribute {
  /* Earliest time at which a host is ready to execute a task */
  double available_at;
  SD_task_t last_scheduled_task;
};

static void sg_host_allocate_attribute(sg_host_t host)
{
  void *data;
  data = calloc(1, sizeof(struct _HostAttribute));
  sg_host_user_set(host, data);
}

static void sg_host_free_attribute(sg_host_t host)
{
  free(sg_host_user(host));
  sg_host_user_set(host, NULL);
}

static double sg_host_get_available_at(sg_host_t host)
{
  HostAttribute attr = (HostAttribute) sg_host_user(host);
  return attr->available_at;
}

static void sg_host_set_available_at(sg_host_t host, double time)
{
  HostAttribute attr = (HostAttribute) sg_host_user(host);
  attr->available_at = time;
  sg_host_user_set(host, attr);
}

static SD_task_t sg_host_get_last_scheduled_task( sg_host_t host){
  HostAttribute attr = (HostAttribute) sg_host_user(host);
  return attr->last_scheduled_task;
}

static void sg_host_set_last_scheduled_task(sg_host_t host, SD_task_t task){
  HostAttribute attr = (HostAttribute) sg_host_user(host);
  attr->last_scheduled_task=task;
  sg_host_user_set(host, attr);
}

static xbt_dynar_t get_ready_tasks(xbt_dynar_t dax)
{
  unsigned int i;
  xbt_dynar_t ready_tasks;
  SD_task_t task;

  ready_tasks = xbt_dynar_new(sizeof(SD_task_t), NULL);
  xbt_dynar_foreach(dax, i, task) {
    if (SD_task_get_kind(task) == SD_TASK_COMP_SEQ && SD_task_get_state(task) == SD_SCHEDULABLE) {
      xbt_dynar_push(ready_tasks, &task);
    }
  }
  XBT_DEBUG("There are %lu ready tasks", xbt_dynar_length(ready_tasks));

  return ready_tasks;
}

static double finish_on_at(SD_task_t task, sg_host_t host)
{
  volatile double result;
  unsigned int i;
  double data_available = 0.;
  double redist_time = 0;
  double last_data_available;
  SD_task_t parent, grand_parent;
  xbt_dynar_t parents, grand_parents;

  sg_host_t *grand_parent_host_list;

  parents = SD_task_get_parents(task);

  if (!xbt_dynar_is_empty(parents)) {
    /* compute last_data_available */
    last_data_available = -1.0;
    xbt_dynar_foreach(parents, i, parent) {
      /* normal case */
      if (SD_task_get_kind(parent) == SD_TASK_COMM_E2E) {
        grand_parents = SD_task_get_parents(parent);

        xbt_assert(xbt_dynar_length(grand_parents) <2, "Error: transfer %s has 2 parents", SD_task_get_name(parent));
        
        xbt_dynar_get_cpy(grand_parents, 0, &grand_parent);

        grand_parent_host_list = SD_task_get_workstation_list(grand_parent);
        /* Estimate the redistribution time from this parent */
        if (SD_task_get_amount(parent) == 0){
          redist_time= 0;
        } else {
          redist_time = SD_route_get_latency(grand_parent_host_list[0], host) +
                        SD_task_get_amount(parent) / SD_route_get_bandwidth(grand_parent_host_list[0], host);
        }
        data_available = SD_task_get_finish_time(grand_parent) + redist_time;

        xbt_dynar_free_container(&grand_parents);
      }

      /* no transfer, control dependency */
      if (SD_task_get_kind(parent) == SD_TASK_COMP_SEQ) {
        data_available = SD_task_get_finish_time(parent);
      }

      if (last_data_available < data_available)
        last_data_available = data_available;
    }

    xbt_dynar_free_container(&parents);

    result = MAX(sg_host_get_available_at(host), last_data_available) + SD_task_get_amount(task)/sg_host_speed(host);
  } else {
    xbt_dynar_free_container(&parents);

    result = sg_host_get_available_at(host) + SD_task_get_amount(task)/sg_host_speed(host);
  }
  return result;
}

static sg_host_t SD_task_get_best_host(SD_task_t task)
{
  int i;
  double EFT, min_EFT = -1.0;
  const sg_host_t *hosts = sg_host_list();
  int nhosts = sg_host_count();
  sg_host_t best_host;

  best_host = hosts[0];
  min_EFT = finish_on_at(task, hosts[0]);

  for (i = 1; i < nhosts; i++) {
    EFT = finish_on_at(task, hosts[i]);
    XBT_DEBUG("%s finishes on %s at %f", SD_task_get_name(task), sg_host_get_name(hosts[i]), EFT);

    if (EFT < min_EFT) {
      min_EFT = EFT;
      best_host = hosts[i];
    }
  }
  return best_host;
}

int main(int argc, char **argv)
{
  unsigned int cursor;
  double finish_time, min_finish_time = -1.0;
  SD_task_t task, selected_task = NULL, last_scheduled_task;
  xbt_dynar_t ready_tasks;
  sg_host_t host, selected_host = NULL;
  int total_nhosts = 0;
  const sg_host_t *hosts = NULL;
  char * tracefilename = NULL;
  xbt_dynar_t dax;

  /* initialization of SD */
  SD_init(&argc, argv);

  /* Check our arguments */
  xbt_assert(argc > 2, "Usage: %s platform_file dax_file [jedule_file]\n"
             "\tExample: %s simulacrum_7_hosts.xml Montage_25.xml Montage_25.jed", argv[0], argv[0]);

  if (argc == 4)
    tracefilename = xbt_strdup(argv[3]);

  /* creation of the environment */
  SD_create_environment(argv[1]);

  /*  Allocating the host attribute */
  total_nhosts = sg_host_count();
  hosts = sg_host_list();

  for (cursor = 0; cursor < total_nhosts; cursor++)
    sg_host_allocate_attribute(hosts[cursor]);

  /* load the DAX file */
  dax = SD_daxload(argv[2]);

  xbt_dynar_foreach(dax, cursor, task) {
    SD_task_watch(task, SD_DONE);
  }

  /* Schedule the root first */
  xbt_dynar_get_cpy(dax, 0, &task);
  host = SD_task_get_best_host(task);
  SD_task_schedulel(task, 1, host);

  while (!xbt_dynar_is_empty(SD_simulate(-1.0))) {
    /* Get the set of ready tasks */
    ready_tasks = get_ready_tasks(dax);
    if (xbt_dynar_is_empty(ready_tasks)) {
      xbt_dynar_free_container(&ready_tasks);
      /* there is no ready task, let advance the simulation */
      continue;
    }
    /* For each ready task:
     * get the host that minimizes the completion time.
     * select the task that has the minimum completion time on its best host.
     */
    xbt_dynar_foreach(ready_tasks, cursor, task) {
      XBT_DEBUG("%s is ready", SD_task_get_name(task));
      host = SD_task_get_best_host(task);
      finish_time = finish_on_at(task, host);
      if (min_finish_time == -1. || finish_time < min_finish_time) {
        min_finish_time = finish_time;
        selected_task = task;
        selected_host = host;
      }
    }

    XBT_INFO("Schedule %s on %s", SD_task_get_name(selected_task), sg_host_get_name(selected_host));
    SD_task_schedulel(selected_task, 1, selected_host);

    /*
     * SimDag allows tasks to be executed concurrently when they can by default.
     * Yet schedulers take decisions assuming that tasks wait for resource availability to start.
     * The solution (well crude hack is to keep track of the last task scheduled on a host and add a special type of
     * dependency if needed to force the sequential execution meant by the scheduler.
     * If the last scheduled task is already done, has failed or is a predecessor of the current task, no need for a
     * new dependency
    */

    last_scheduled_task = sg_host_get_last_scheduled_task(selected_host);
    if (last_scheduled_task && (SD_task_get_state(last_scheduled_task) != SD_DONE) &&
        (SD_task_get_state(last_scheduled_task) != SD_FAILED) &&
        !SD_task_dependency_exists(sg_host_get_last_scheduled_task(selected_host), selected_task))
      SD_task_dependency_add("resource", NULL, last_scheduled_task, selected_task);

    sg_host_set_last_scheduled_task(selected_host, selected_task);
    sg_host_set_available_at(selected_host, min_finish_time);

    xbt_dynar_free_container(&ready_tasks);
    /* reset the min_finish_time for the next set of ready tasks */
    min_finish_time = -1.;
  }

  XBT_INFO("Simulation Time: %f", SD_get_clock());
  XBT_INFO("------------------- Produce the trace file---------------------------");
  XBT_INFO("Producing a jedule output (if active) of the run into %s", tracefilename?tracefilename:"minmin_test.jed");
#if HAVE_JEDULE
  jedule_sd_dump(tracefilename);
#endif
  free(tracefilename);

  xbt_dynar_free_container(&ready_tasks);

  xbt_dynar_foreach(dax, cursor, task) {
    SD_task_destroy(task);
  }
  xbt_dynar_free_container(&dax);

  for (cursor = 0; cursor < total_nhosts; cursor++)
    sg_host_free_attribute(hosts[cursor]);

  /* exit */
  SD_exit();
  return 0;
}
