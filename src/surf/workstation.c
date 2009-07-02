/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "xbt/dict.h"
#include "portable.h"
#include "surf_private.h"
#include "network_common.h"

typedef struct workstation_CLM03 {
  s_surf_resource_t generic_resource; /* Must remain first to add this to a trace */
  void *cpu;
  int id;
} s_workstation_CLM03_t, *workstation_CLM03_t;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_workstation, surf,
                                "Logging specific to the SURF workstation module");

surf_model_t surf_workstation_model = NULL;

static workstation_CLM03_t workstation_new(const char *name,
                                           void *cpu, int id)
{
  workstation_CLM03_t workstation = xbt_new0(s_workstation_CLM03_t, 1);

  workstation->generic_resource.model = surf_workstation_model;
  workstation->generic_resource.name = xbt_strdup(name);
  workstation->cpu = cpu;
  workstation->id = id;

  xbt_dict_set(surf_model_resource_set(surf_workstation_model), name,
               workstation, surf_resource_free);

  return workstation;
}

void create_workstations(void)
{
  xbt_dict_cursor_t cursor = NULL;
  char *name = NULL;
  void *cpu = NULL;

  xbt_dict_foreach(surf_model_resource_set(surf_cpu_model), cursor, name, cpu) {
    int *id = xbt_dict_get_or_null(used_routing->host_id,name);
    xbt_assert1(id, "No host %s found in the platform file", name);

    workstation_new(name, cpu, *id);
  }
}

static int resource_used(void *resource_id)
{
  THROW_IMPOSSIBLE;             /* This model does not implement parallel tasks */
}

static void parallel_action_cancel(surf_action_t action)
{
  THROW_UNIMPLEMENTED;          /* This model does not implement parallel tasks */
}

static int parallel_action_free(surf_action_t action)
{
  THROW_UNIMPLEMENTED;          /* This model does not implement parallel tasks */
}

static int action_unref(surf_action_t action)
{
  if (action->model_type == surf_network_model)
    return surf_network_model->action_unref(action);
  else if (action->model_type == surf_cpu_model)
    return surf_cpu_model->action_unref(action);
  else if (action->model_type == surf_workstation_model)
    return parallel_action_free(action);
  else
    DIE_IMPOSSIBLE;
  return 0;
}

static void action_cancel(surf_action_t action)
{
  if (action->model_type == surf_network_model)
    surf_network_model->action_cancel(action);
  else if (action->model_type == surf_cpu_model)
    surf_cpu_model->action_cancel(action);
  else if (action->model_type == surf_workstation_model)
    parallel_action_cancel(action);
  else
    DIE_IMPOSSIBLE;
  return;
}

static void ws_action_state_set(surf_action_t action,
                                e_surf_action_state_t state)
{
  if (action->model_type == surf_network_model)
    surf_network_model->action_state_set(action, state);
  else if (action->model_type == surf_cpu_model)
    surf_cpu_model->action_state_set(action, state);
  else if (action->model_type == surf_workstation_model)
    surf_action_state_set(action, state);
  else
    DIE_IMPOSSIBLE;
  return;
}

static double share_resources(double now)
{
  return -1.0;
}

static void update_actions_state(double now, double delta)
{
  return;
}

static void update_resource_state(void *id,
                                  tmgr_trace_event_t event_type,
                                  double value, double date)
{
  THROW_IMPOSSIBLE;             /* This model does not implement parallel tasks */
}

static surf_action_t execute(void *workstation, double size)
{
  return surf_cpu_model->extension.
    cpu.execute(((workstation_CLM03_t) workstation)->cpu, size);
}

static surf_action_t action_sleep(void *workstation, double duration)
{
  return surf_cpu_model->extension.
    cpu.sleep(((workstation_CLM03_t) workstation)->cpu, duration);
}

static void action_suspend(surf_action_t action)
{
  if (action->model_type == surf_network_model)
    surf_network_model->suspend(action);
  else if (action->model_type == surf_cpu_model)
    surf_cpu_model->suspend(action);
  else
    DIE_IMPOSSIBLE;
}

static void action_resume(surf_action_t action)
{
  if (action->model_type == surf_network_model)
    surf_network_model->resume(action);
  else if (action->model_type == surf_cpu_model)
    surf_cpu_model->resume(action);
  else
    DIE_IMPOSSIBLE;
}

static int action_is_suspended(surf_action_t action)
{
  if (action->model_type == surf_network_model)
    return surf_network_model->is_suspended(action);
  if (action->model_type == surf_cpu_model)
    return surf_cpu_model->is_suspended(action);
  DIE_IMPOSSIBLE;
}

static void action_set_max_duration(surf_action_t action, double duration)
{
  if (action->model_type == surf_network_model)
    surf_network_model->set_max_duration(action, duration);
  else if (action->model_type == surf_cpu_model)
    surf_cpu_model->set_max_duration(action, duration);
  else
    DIE_IMPOSSIBLE;
}

static void action_set_priority(surf_action_t action, double priority)
{
  if (action->model_type == surf_network_model)
    surf_network_model->set_priority(action, priority);
  else if (action->model_type == surf_cpu_model)
    surf_cpu_model->set_priority(action, priority);
  else
    DIE_IMPOSSIBLE;
}

static surf_action_t communicate(void *workstation_src,
                                 void *workstation_dst, double size,
                                 double rate)
{
  workstation_CLM03_t src = (workstation_CLM03_t) workstation_src;
  workstation_CLM03_t dst = (workstation_CLM03_t) workstation_dst;
  return surf_network_model->extension.
    network.communicate(surf_resource_name(src->cpu),surf_resource_name(dst->cpu),
        src->id,dst->id,
        size, rate);
}

static e_surf_cpu_state_t get_state(void *workstation)
{
  return surf_cpu_model->extension.
    cpu.get_state(((workstation_CLM03_t) workstation)->cpu);
}

static double get_speed(void *workstation, double load)
{
  return surf_cpu_model->extension.
    cpu.get_speed(((workstation_CLM03_t) workstation)->cpu, load);
}

static double get_available_speed(void *workstation)
{
  return surf_cpu_model->extension.
    cpu.get_available_speed(((workstation_CLM03_t)
                             workstation)->cpu);
}

static surf_action_t execute_parallel_task(int workstation_nb,
                                           void **workstation_list,
                                           double *computation_amount,
                                           double *communication_amount,
                                           double amount, double rate)
{
  THROW_UNIMPLEMENTED;          /* This model does not implement parallel tasks */
}


/* returns an array of network_link_CM02_t */
static xbt_dynar_t get_route(void *src, void *dst)
{
  workstation_CLM03_t workstation_src = (workstation_CLM03_t) src;
  workstation_CLM03_t workstation_dst = (workstation_CLM03_t) dst;
  return surf_network_model->extension.network.get_route(workstation_src->id,
                                                         workstation_dst->id);
}

static double get_link_bandwidth(const void *link)
{
  return surf_network_model->extension.network.get_link_bandwidth(link);
}

static double get_link_latency(const void *link)
{
  return surf_network_model->extension.network.get_link_latency(link);
}

static int link_shared(const void *link)
{
  return surf_network_model->extension.network.get_link_latency(link);
}

static void finalize(void)
{
  surf_model_exit(surf_workstation_model);
  surf_workstation_model = NULL;
}

static void surf_workstation_model_init_internal(void)
{
  surf_workstation_model = surf_model_init();

  surf_workstation_model->name = "Workstation";
  surf_workstation_model->action_unref = action_unref;
  surf_workstation_model->action_cancel = action_cancel;
  surf_workstation_model->action_state_set = ws_action_state_set;

  surf_workstation_model->model_private->resource_used = resource_used;
  surf_workstation_model->model_private->share_resources = share_resources;
  surf_workstation_model->model_private->update_actions_state =
    update_actions_state;
  surf_workstation_model->model_private->update_resource_state =
    update_resource_state;
  surf_workstation_model->model_private->finalize = finalize;

  surf_workstation_model->suspend = action_suspend;
  surf_workstation_model->resume = action_resume;
  surf_workstation_model->is_suspended = action_is_suspended;
  surf_workstation_model->set_max_duration = action_set_max_duration;
  surf_workstation_model->set_priority = action_set_priority;

  surf_workstation_model->extension.workstation.execute = execute;
  surf_workstation_model->extension.workstation.sleep = action_sleep;
  surf_workstation_model->extension.workstation.get_state = get_state;
  surf_workstation_model->extension.workstation.get_speed = get_speed;
  surf_workstation_model->extension.workstation.get_available_speed =
    get_available_speed;

  surf_workstation_model->extension.workstation.communicate = communicate;
  surf_workstation_model->extension.workstation.get_route = get_route;
  surf_workstation_model->extension.workstation.execute_parallel_task =
    execute_parallel_task;
  surf_workstation_model->extension.workstation.get_link_bandwidth =
    get_link_bandwidth;
  surf_workstation_model->extension.workstation.get_link_latency =
    get_link_latency;
  surf_workstation_model->extension.workstation.link_shared = link_shared;
}

/********************************************************************/
/* The model used in MSG and presented at CCGrid03                  */
/********************************************************************/
/* @InProceedings{Casanova.CLM_03, */
/*   author = {Henri Casanova and Arnaud Legrand and Loris Marchal}, */
/*   title = {Scheduling Distributed Applications: the SimGrid Simulation Framework}, */
/*   booktitle = {Proceedings of the third IEEE International Symposium on Cluster Computing and the Grid (CCGrid'03)}, */
/*   publisher = {"IEEE Computer Society Press"}, */
/*   month = {may}, */
/*   year = {2003} */
/* } */
void surf_workstation_model_init_CLM03(const char *filename)
{
  surf_workstation_model_init_internal();
  surf_cpu_model_init_Cas01(filename);
  surf_network_model_init_CM02(filename);
  update_model_description(surf_workstation_model_description,
                           "CLM03", surf_workstation_model);
  xbt_dynar_push(model_list, &surf_workstation_model);
}

void surf_workstation_model_init_compound(const char *filename)
{

  xbt_assert0(surf_cpu_model, "No CPU model defined yet!");
  xbt_assert0(surf_network_model, "No network model defined yet!");
  surf_workstation_model_init_internal();

  update_model_description(surf_workstation_model_description,
                           "compound", surf_workstation_model);

  xbt_dynar_push(model_list, &surf_workstation_model);
}
