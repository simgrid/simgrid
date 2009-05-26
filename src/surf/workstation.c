/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/ex.h"
#include "xbt/dict.h"
#include "portable.h"
#include "workstation_private.h"
#include "cpu_private.h"
#include "network_common.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_workstation, surf,
                                "Logging specific to the SURF workstation module");

surf_workstation_model_t surf_workstation_model = NULL;
xbt_dict_t workstation_set = NULL;

static workstation_CLM03_t workstation_new(const char *name,
                                           void *cpu, void *card)
{
  workstation_CLM03_t workstation = xbt_new0(s_workstation_CLM03_t, 1);

  workstation->model = (surf_model_t) surf_workstation_model;
  workstation->name = xbt_strdup(name);
  workstation->cpu = cpu;
  workstation->network_card = card;

  return workstation;
}

static void workstation_free(void *workstation)
{
  free(((workstation_CLM03_t) workstation)->name);
  free(workstation);
}

void create_workstations(void)
{
  xbt_dict_cursor_t cursor = NULL;
  char *name = NULL;
  void *cpu = NULL;
  void *nw_card = NULL;

  xbt_dict_foreach(cpu_set, cursor, name, cpu) {
    nw_card = xbt_dict_get_or_null(network_card_set, name);
    xbt_assert1(nw_card, "No corresponding card found for %s", name);

    xbt_dict_set(workstation_set, name,
                 workstation_new(name, cpu, nw_card), workstation_free);
  }
}

static void *name_service(const char *name)
{
  return xbt_dict_get_or_null(workstation_set, name);
}

static const char *get_resource_name(void *resource_id)
{
  return ((workstation_CLM03_t) resource_id)->name;
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

static void parallel_action_use(surf_action_t action)
{
  THROW_UNIMPLEMENTED;          /* This model does not implement parallel tasks */
}

static int action_free(surf_action_t action)
{
  if (action->model_type == (surf_model_t) surf_network_model)
    return surf_network_model->common_public->action_free(action);
  else if (action->model_type == (surf_model_t) surf_cpu_model)
    return surf_cpu_model->common_public->action_free(action);
  else if (action->model_type == (surf_model_t) surf_workstation_model)
    return parallel_action_free(action);
  else
    DIE_IMPOSSIBLE;
  return 0;
}

static void action_use(surf_action_t action)
{
  if (action->model_type == (surf_model_t) surf_network_model)
    surf_network_model->common_public->action_use(action);
  else if (action->model_type == (surf_model_t) surf_cpu_model)
    surf_cpu_model->common_public->action_use(action);
  else if (action->model_type == (surf_model_t) surf_workstation_model)
    parallel_action_use(action);
  else
    DIE_IMPOSSIBLE;
  return;
}

static void action_cancel(surf_action_t action)
{
  if (action->model_type == (surf_model_t) surf_network_model)
    surf_network_model->common_public->action_cancel(action);
  else if (action->model_type == (surf_model_t) surf_cpu_model)
    surf_cpu_model->common_public->action_cancel(action);
  else if (action->model_type == (surf_model_t) surf_workstation_model)
    parallel_action_cancel(action);
  else
    DIE_IMPOSSIBLE;
  return;
}

static void action_recycle(surf_action_t action)
{
  DIE_IMPOSSIBLE;
  return;
}

static void action_change_state(surf_action_t action,
                                e_surf_action_state_t state)
{
  if (action->model_type == (surf_model_t) surf_network_model)
    surf_network_model->common_public->action_change_state(action, state);
  else if (action->model_type == (surf_model_t) surf_cpu_model)
    surf_cpu_model->common_public->action_change_state(action, state);
  else if (action->model_type == (surf_model_t) surf_workstation_model)
    surf_action_change_state(action, state);
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
  return surf_cpu_model->
    extension_public->execute(((workstation_CLM03_t) workstation)->cpu, size);
}

static surf_action_t action_sleep(void *workstation, double duration)
{
  return surf_cpu_model->
    extension_public->sleep(((workstation_CLM03_t) workstation)->cpu,
                            duration);
}

static void action_suspend(surf_action_t action)
{
  if (action->model_type == (surf_model_t) surf_network_model)
    surf_network_model->common_public->suspend(action);
  else if (action->model_type == (surf_model_t) surf_cpu_model)
    surf_cpu_model->common_public->suspend(action);
  else
    DIE_IMPOSSIBLE;
}

static void action_resume(surf_action_t action)
{
  if (action->model_type == (surf_model_t) surf_network_model)
    surf_network_model->common_public->resume(action);
  else if (action->model_type == (surf_model_t) surf_cpu_model)
    surf_cpu_model->common_public->resume(action);
  else
    DIE_IMPOSSIBLE;
}

static int action_is_suspended(surf_action_t action)
{
  if (action->model_type == (surf_model_t) surf_network_model)
    return surf_network_model->common_public->is_suspended(action);
  if (action->model_type == (surf_model_t) surf_cpu_model)
    return surf_cpu_model->common_public->is_suspended(action);
  DIE_IMPOSSIBLE;
}

static void action_set_max_duration(surf_action_t action, double duration)
{
  if (action->model_type == (surf_model_t) surf_network_model)
    surf_network_model->common_public->set_max_duration(action, duration);
  else if (action->model_type == (surf_model_t) surf_cpu_model)
    surf_cpu_model->common_public->set_max_duration(action, duration);
  else
    DIE_IMPOSSIBLE;
}

static void action_set_priority(surf_action_t action, double priority)
{
  if (action->model_type == (surf_model_t) surf_network_model)
    surf_network_model->common_public->set_priority(action, priority);
  else if (action->model_type == (surf_model_t) surf_cpu_model)
    surf_cpu_model->common_public->set_priority(action, priority);
  else
    DIE_IMPOSSIBLE;
}

static surf_action_t communicate(void *workstation_src,
                                 void *workstation_dst, double size,
                                 double rate)
{
  return surf_network_model->
    extension_public->communicate(((workstation_CLM03_t) workstation_src)->
                                  network_card,
                                  ((workstation_CLM03_t) workstation_dst)->
                                  network_card, size, rate);
}

static e_surf_cpu_state_t get_state(void *workstation)
{
  return surf_cpu_model->
    extension_public->get_state(((workstation_CLM03_t) workstation)->cpu);
}

static double get_speed(void *workstation, double load)
{
  return surf_cpu_model->
    extension_public->get_speed(((workstation_CLM03_t) workstation)->cpu,
                                load);
}

static double get_available_speed(void *workstation)
{
  return surf_cpu_model->
    extension_public->get_available_speed(((workstation_CLM03_t)
                                           workstation)->cpu);
}

static xbt_dict_t get_properties(void *workstation)
{
  return surf_cpu_model->
    common_public->get_properties(((workstation_CLM03_t) workstation)->cpu);
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
static const void **get_route(void *src, void *dst)
{
  workstation_CLM03_t workstation_src = (workstation_CLM03_t) src;
  workstation_CLM03_t workstation_dst = (workstation_CLM03_t) dst;
  return surf_network_model->extension_public->get_route(workstation_src->
                                                         network_card,
                                                         workstation_dst->
                                                         network_card);
}

static int get_route_size(void *src, void *dst)
{
  workstation_CLM03_t workstation_src = (workstation_CLM03_t) src;
  workstation_CLM03_t workstation_dst = (workstation_CLM03_t) dst;
  return surf_network_model->
    extension_public->get_route_size(workstation_src->network_card,
                                     workstation_dst->network_card);
}

static const char *get_link_name(const void *link)
{
  return surf_network_model->extension_public->get_link_name(link);
}

static double get_link_bandwidth(const void *link)
{
  return surf_network_model->extension_public->get_link_bandwidth(link);
}

static double get_link_latency(const void *link)
{
  return surf_network_model->extension_public->get_link_latency(link);
}

static int link_shared(const void *link)
{
  return surf_network_model->extension_public->get_link_latency(link);
}

static void finalize(void)
{
  xbt_dict_free(&workstation_set);
  xbt_swag_free(surf_workstation_model->common_public->
                states.ready_action_set);
  xbt_swag_free(surf_workstation_model->common_public->
                states.running_action_set);
  xbt_swag_free(surf_workstation_model->common_public->
                states.failed_action_set);
  xbt_swag_free(surf_workstation_model->common_public->
                states.done_action_set);

  free(surf_workstation_model->common_public);
  free(surf_workstation_model->common_private);
  free(surf_workstation_model->extension_public);

  free(surf_workstation_model);
  surf_workstation_model = NULL;
}

static void surf_workstation_model_init_internal(void)
{
  s_surf_action_t action;

  surf_workstation_model = xbt_new0(s_surf_workstation_model_t, 1);

  surf_workstation_model->common_private =
    xbt_new0(s_surf_model_private_t, 1);
  surf_workstation_model->common_public = xbt_new0(s_surf_model_public_t, 1);
/*   surf_workstation_model->extension_private = xbt_new0(s_surf_workstation_model_extension_private_t,1); */
  surf_workstation_model->extension_public =
    xbt_new0(s_surf_workstation_model_extension_public_t, 1);

  surf_workstation_model->common_public->states.ready_action_set =
    xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_workstation_model->common_public->states.running_action_set =
    xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_workstation_model->common_public->states.failed_action_set =
    xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_workstation_model->common_public->states.done_action_set =
    xbt_swag_new(xbt_swag_offset(action, state_hookup));

  surf_workstation_model->common_public->name_service = name_service;
  surf_workstation_model->common_public->get_resource_name =
    get_resource_name;
  surf_workstation_model->common_public->action_get_state =
    surf_action_get_state;
  surf_workstation_model->common_public->action_get_start_time =
    surf_action_get_start_time;
  surf_workstation_model->common_public->action_get_finish_time =
    surf_action_get_finish_time;
  surf_workstation_model->common_public->action_free = action_free;
  surf_workstation_model->common_public->action_use = action_use;
  surf_workstation_model->common_public->action_cancel = action_cancel;
  surf_workstation_model->common_public->action_recycle = action_recycle;
  surf_workstation_model->common_public->action_change_state =
    action_change_state;
  surf_workstation_model->common_public->action_set_data =
    surf_action_set_data;
  surf_workstation_model->common_public->name = "Workstation";

  surf_workstation_model->common_private->resource_used = resource_used;
  surf_workstation_model->common_private->share_resources = share_resources;
  surf_workstation_model->common_private->update_actions_state =
    update_actions_state;
  surf_workstation_model->common_private->update_resource_state =
    update_resource_state;
  surf_workstation_model->common_private->finalize = finalize;

  surf_workstation_model->common_public->suspend = action_suspend;
  surf_workstation_model->common_public->resume = action_resume;
  surf_workstation_model->common_public->is_suspended = action_is_suspended;
  surf_workstation_model->common_public->set_max_duration =
    action_set_max_duration;
  surf_workstation_model->common_public->set_priority = action_set_priority;

  surf_workstation_model->extension_public->execute = execute;
  surf_workstation_model->extension_public->sleep = action_sleep;
  surf_workstation_model->extension_public->get_state = get_state;
  surf_workstation_model->extension_public->get_speed = get_speed;
  surf_workstation_model->extension_public->get_available_speed =
    get_available_speed;

  /*manage the properties of the workstation */
  surf_workstation_model->common_public->get_properties = get_properties;

  surf_workstation_model->extension_public->communicate = communicate;
  surf_workstation_model->extension_public->execute_parallel_task =
    execute_parallel_task;
  surf_workstation_model->extension_public->get_route = get_route;
  surf_workstation_model->extension_public->get_route_size = get_route_size;
  surf_workstation_model->extension_public->get_link_name = get_link_name;
  surf_workstation_model->extension_public->get_link_bandwidth =
    get_link_bandwidth;
  surf_workstation_model->extension_public->get_link_latency =
    get_link_latency;
  surf_workstation_model->extension_public->link_shared = link_shared;

  workstation_set = xbt_dict_new();
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
                           "CLM03", (surf_model_t) surf_workstation_model);
  xbt_dynar_push(model_list, &surf_workstation_model);
}

void surf_workstation_model_init_compound(const char *filename)
{

  xbt_assert0(surf_cpu_model, "No CPU model defined yet!");
  xbt_assert0(surf_network_model, "No network model defined yet!");
  surf_workstation_model_init_internal();

  update_model_description(surf_workstation_model_description,
                           "compound", (surf_model_t) surf_workstation_model);

  xbt_dynar_push(model_list, &surf_workstation_model);
}
