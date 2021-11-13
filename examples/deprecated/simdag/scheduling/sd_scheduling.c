/* Copyright (c) 2009-2021. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* simple test to schedule a DAX file with the Min-Min algorithm.           */
#include "simgrid/simdag.h"
#include <math.h>
#include <string.h>

#if SIMGRID_HAVE_JEDULE
#include "simgrid/jedule/jedule_sd_binding.h"
#endif

XBT_LOG_NEW_DEFAULT_CATEGORY(test, "Logging specific to this SimDag example");

typedef struct _HostAttribute *HostAttribute;
struct _HostAttribute {
  /* Earliest time at which a host is ready to execute a task */
  double available_at;
  SD_task_t last_scheduled_task;
};

static double sg_host_get_available_at(const_sg_host_t host)
{
  const struct _HostAttribute* attr = (HostAttribute)sg_host_get_data(host);
  return attr->available_at;
}

static void sg_host_set_available_at(sg_host_t host, double time)
{
  HostAttribute attr = (HostAttribute)sg_host_get_data(host);
  attr->available_at = time;
  sg_host_set_data(host, attr);
}

static SD_task_t sg_host_get_last_scheduled_task(const_sg_host_t host)
{
  const struct _HostAttribute* attr = (HostAttribute)sg_host_get_data(host);
  return attr->last_scheduled_task;
}

static void sg_host_set_last_scheduled_task(sg_host_t host, SD_task_t task){
  HostAttribute attr       = (HostAttribute)sg_host_get_data(host);
  attr->last_scheduled_task=task;
  sg_host_set_data(host, attr);
}

static xbt_dynar_t get_ready_tasks(const_xbt_dynar_t dax)
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

static double finish_on_at(const_SD_task_t task, const_sg_host_t host)
{
  double result;

  xbt_dynar_t parents = SD_task_get_parents(task);

  if (!xbt_dynar_is_empty(parents)) {
    unsigned int i;
    double data_available = 0.;
    double last_data_available;
    /* compute last_data_available */
    SD_task_t parent;
    last_data_available = -1.0;
    xbt_dynar_foreach(parents, i, parent) {
      /* normal case */
      if (SD_task_get_kind(parent) == SD_TASK_COMM_E2E) {
        sg_host_t * parent_host= SD_task_get_workstation_list(parent);
        /* Estimate the redistribution time from this parent */
        double redist_time;
        if (SD_task_get_amount(parent) <= 1e-6){
          redist_time= 0;
        } else {
          redist_time = sg_host_get_route_latency(parent_host[0], host) +
                        SD_task_get_amount(parent) / sg_host_get_route_bandwidth(parent_host[0], host);
        }
        data_available = SD_task_get_start_time(parent) + redist_time;
      }

      /* no transfer, control dependency */
      if (SD_task_get_kind(parent) == SD_TASK_COMP_SEQ) {
        data_available = SD_task_get_finish_time(parent);
      }

      if (last_data_available < data_available)
        last_data_available = data_available;
    }

    xbt_dynar_free_container(&parents);

    result =
        fmax(sg_host_get_available_at(host), last_data_available) + SD_task_get_amount(task) / sg_host_get_speed(host);
  } else {
    xbt_dynar_free_container(&parents);

    result = sg_host_get_available_at(host) + SD_task_get_amount(task) / sg_host_get_speed(host);
  }
  return result;
}

static sg_host_t SD_task_get_best_host(const_SD_task_t task)
{
  sg_host_t *hosts = sg_host_list();
  int nhosts = sg_host_count();
  sg_host_t best_host = hosts[0];
  double min_EFT = finish_on_at(task, hosts[0]);

  for (int i = 1; i < nhosts; i++) {
    double EFT = finish_on_at(task, hosts[i]);
    XBT_DEBUG("%s finishes on %s at %f", SD_task_get_name(task), sg_host_get_name(hosts[i]), EFT);

    if (EFT < min_EFT) {
      min_EFT = EFT;
      best_host = hosts[i];
    }
  }
  xbt_free(hosts);
  return best_host;
}

int main(int argc, char **argv)
{
  unsigned int cursor;
  double min_finish_time = -1.0;
  SD_task_t task;
  SD_task_t selected_task = NULL;
  xbt_dynar_t ready_tasks;
  sg_host_t selected_host = NULL;
  char * tracefilename = NULL;

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
  unsigned int total_nhosts = sg_host_count();
  sg_host_t *hosts = sg_host_list();

  for (cursor = 0; cursor < total_nhosts; cursor++)
    sg_host_set_data(hosts[cursor], xbt_new0(struct _HostAttribute, 1));

  /* load the DAX file */
  xbt_dynar_t dax = SD_daxload(argv[2]);

  xbt_dynar_foreach(dax, cursor, task) {
    SD_task_watch(task, SD_DONE);
  }

  /* Schedule the root first */
  xbt_dynar_get_cpy(dax, 0, &task);
  sg_host_t host = SD_task_get_best_host(task);
  SD_task_schedulel(task, 1, host);
  xbt_dynar_t changed_tasks = xbt_dynar_new(sizeof(SD_task_t), NULL);
  SD_simulate_with_update(-1.0, changed_tasks);

  while (!xbt_dynar_is_empty(changed_tasks)) {
    /* Get the set of ready tasks */
    ready_tasks = get_ready_tasks(dax);
    xbt_dynar_reset(changed_tasks);

    if (xbt_dynar_is_empty(ready_tasks)) {
      xbt_dynar_free_container(&ready_tasks);
      /* there is no ready task, let advance the simulation */
      SD_simulate_with_update(-1.0, changed_tasks);
      continue;
    }
    /* For each ready task:
     * get the host that minimizes the completion time.
     * select the task that has the minimum completion time on its best host.
     */
    xbt_dynar_foreach(ready_tasks, cursor, task) {
      XBT_DEBUG("%s is ready", SD_task_get_name(task));
      host = SD_task_get_best_host(task);
      double finish_time = finish_on_at(task, host);
      if (min_finish_time < 0 || finish_time < min_finish_time) {
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

    SD_task_t last_scheduled_task = sg_host_get_last_scheduled_task(selected_host);
    if (last_scheduled_task && (SD_task_get_state(last_scheduled_task) != SD_DONE) &&
        (SD_task_get_state(last_scheduled_task) != SD_FAILED) &&
        !SD_task_dependency_exists(sg_host_get_last_scheduled_task(selected_host), selected_task))
      SD_task_dependency_add(last_scheduled_task, selected_task);

    sg_host_set_last_scheduled_task(selected_host, selected_task);
    sg_host_set_available_at(selected_host, min_finish_time);

    xbt_dynar_free_container(&ready_tasks);
    /* reset the min_finish_time for the next set of ready tasks */
    min_finish_time = -1.;
    xbt_dynar_reset(changed_tasks);
    SD_simulate_with_update(-1.0, changed_tasks);
  }

  XBT_INFO("Simulation Time: %f", simgrid_get_clock());
  XBT_INFO("------------------- Produce the trace file---------------------------");
  XBT_INFO("Producing a jedule output (if active) of the run into %s", tracefilename?tracefilename:"minmin_test.jed");
#if SIMGRID_HAVE_JEDULE
  jedule_sd_dump(tracefilename);
#endif
  free(tracefilename);

  xbt_dynar_free_container(&ready_tasks);
  xbt_dynar_free(&changed_tasks);

  xbt_dynar_foreach(dax, cursor, task) {
    SD_task_destroy(task);
  }
  xbt_dynar_free_container(&dax);

  for (cursor = 0; cursor < total_nhosts; cursor++) {
    free(sg_host_get_data(hosts[cursor]));
    sg_host_set_data(hosts[cursor], NULL);
  }

  xbt_free(hosts);
  return 0;
}
