/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/dict.h"
#include "workstation_private.h"
#include "cpu_private.h"
#include "network_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(workstation, surf,
				"Logging specific to the SURF workstation module");

surf_workstation_resource_t surf_workstation_resource = NULL;

xbt_dict_t workstation_set = NULL;

static workstation_CLM03_t workstation_new(const char *name,
				     void *cpu, void *card)
{
  workstation_CLM03_t workstation = xbt_new0(s_workstation_CLM03_t, 1);

  workstation->resource = (surf_resource_t) surf_workstation_resource;
  workstation->name = xbt_strdup(name);
  workstation->cpu = cpu;
  workstation->network_card = card;

  return workstation;
}

static void workstation_free(void *workstation)
{
  free(((workstation_CLM03_t)workstation)->name);
  free(workstation);
}

static void create_workstations(void)
{
  xbt_dict_cursor_t cursor = NULL;
  char *name = NULL;
  void *cpu = NULL;
  void *nw_card = NULL;

  xbt_dict_foreach(cpu_set, cursor, name, cpu) {
    nw_card = NULL;
    xbt_dict_get(network_card_set, name, (void *) &nw_card);
    xbt_assert1(nw_card, "No corresponding card found for %s",name);
    xbt_dict_set(workstation_set, name,
		 workstation_new(name, cpu, nw_card), workstation_free);
  }
}

static void *name_service(const char *name)
{
  void *workstation = NULL;

  xbt_dict_get(workstation_set, name, &workstation);

  return workstation;
}

static const char *get_resource_name(void *resource_id)
{
  return ((workstation_CLM03_t) resource_id)->name;
}

static int resource_used(void *resource_id)
{
  xbt_assert0(0,
	      "Workstation is a virtual resource. I should not be there!");
  return 0;
}

static void action_free(surf_action_t action)
{
  if(action->resource_type==(surf_resource_t)surf_network_resource) 
    surf_network_resource->common_public->action_free(action);
  else if(action->resource_type==(surf_resource_t)surf_cpu_resource) 
    surf_cpu_resource->common_public->action_free(action);
  else DIE_IMPOSSIBLE;
  return;
}

static void action_cancel(surf_action_t action)
{
  return;
}

static void action_recycle(surf_action_t action)
{
  return;
}

static void action_change_state(surf_action_t action,
				e_surf_action_state_t state)
{
  if(action->resource_type==(surf_resource_t)surf_network_resource) 
    surf_network_resource->common_public->action_change_state(action,state);
  else if(action->resource_type==(surf_resource_t)surf_cpu_resource) 
    surf_cpu_resource->common_public->action_change_state(action,state);
  else DIE_IMPOSSIBLE;
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
				  double value)
{
  return;
}

static surf_action_t execute(void *workstation, double size)
{
  return surf_cpu_resource->extension_public->
      execute(((workstation_CLM03_t) workstation)->cpu, size);
}

static surf_action_t action_sleep(void *workstation, double duration)
{
  return surf_cpu_resource->extension_public->
      sleep(((workstation_CLM03_t) workstation)->cpu, duration);
}

static void action_suspend(surf_action_t action)
{
  if(action->resource_type==(surf_resource_t)surf_network_resource) 
    surf_network_resource->common_public->suspend(action);
  else if(action->resource_type==(surf_resource_t)surf_cpu_resource) 
    surf_cpu_resource->common_public->suspend(action);
  else DIE_IMPOSSIBLE;
}

static void action_resume(surf_action_t action)
{
  if(action->resource_type==(surf_resource_t)surf_network_resource)
    surf_network_resource->common_public->resume(action);
  else if(action->resource_type==(surf_resource_t)surf_cpu_resource)
    surf_cpu_resource->common_public->resume(action);
  else DIE_IMPOSSIBLE;
}

static int action_is_suspended(surf_action_t action)
{
  if(action->resource_type==(surf_resource_t)surf_network_resource) 
    return surf_network_resource->common_public->is_suspended(action);
  if(action->resource_type==(surf_resource_t)surf_cpu_resource) 
    return surf_cpu_resource->common_public->is_suspended(action);
  DIE_IMPOSSIBLE;
}

static surf_action_t communicate(void *workstation_src,
				 void *workstation_dst, double size,
				 double rate)
{
  return surf_network_resource->extension_public->
      communicate(((workstation_CLM03_t) workstation_src)->network_card,
		  ((workstation_CLM03_t) workstation_dst)->network_card, size, rate);
}

static e_surf_cpu_state_t get_state(void *workstation)
{
  return surf_cpu_resource->extension_public->
      get_state(((workstation_CLM03_t) workstation)->cpu);
}

static void finalize(void)
{
  xbt_dict_free(&workstation_set);
  xbt_swag_free(surf_workstation_resource->common_public->states.ready_action_set);
  xbt_swag_free(surf_workstation_resource->common_public->states.
		running_action_set);
  xbt_swag_free(surf_workstation_resource->common_public->states.
		failed_action_set);
  xbt_swag_free(surf_workstation_resource->common_public->states.done_action_set);

  free(surf_workstation_resource->common_public);
  free(surf_workstation_resource->common_private);
  free(surf_workstation_resource->extension_public);

  free(surf_workstation_resource);
  surf_workstation_resource = NULL;
}

static void surf_workstation_resource_init_internal(void)
{
  s_surf_action_t action;

  surf_workstation_resource = xbt_new0(s_surf_workstation_resource_t, 1);

  surf_workstation_resource->common_private =
      xbt_new0(s_surf_resource_private_t, 1);
  surf_workstation_resource->common_public =
      xbt_new0(s_surf_resource_public_t, 1);
/*   surf_workstation_resource->extension_private = xbt_new0(s_surf_workstation_resource_extension_private_t,1); */
  surf_workstation_resource->extension_public =
      xbt_new0(s_surf_workstation_resource_extension_public_t, 1);

  surf_workstation_resource->common_public->states.ready_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_workstation_resource->common_public->states.running_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_workstation_resource->common_public->states.failed_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_workstation_resource->common_public->states.done_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));

  surf_workstation_resource->common_public->name_service = name_service;
  surf_workstation_resource->common_public->get_resource_name =
      get_resource_name;
  surf_workstation_resource->common_public->action_get_state =
      surf_action_get_state;
  surf_workstation_resource->common_public->action_free = action_free;
  surf_workstation_resource->common_public->action_cancel = action_cancel;
  surf_workstation_resource->common_public->action_recycle =
      action_recycle;
  surf_workstation_resource->common_public->action_change_state =
      action_change_state;
  surf_workstation_resource->common_public->action_set_data = surf_action_set_data;
  surf_workstation_resource->common_public->name = "Workstation";

  surf_workstation_resource->common_private->resource_used = resource_used;
  surf_workstation_resource->common_private->share_resources =
      share_resources;
  surf_workstation_resource->common_private->update_actions_state =
      update_actions_state;
  surf_workstation_resource->common_private->update_resource_state =
      update_resource_state;
  surf_workstation_resource->common_private->finalize = finalize;

  surf_workstation_resource->common_public->suspend = action_suspend;
  surf_workstation_resource->common_public->resume = action_resume;
  surf_workstation_resource->common_public->is_suspended = action_is_suspended;

  surf_workstation_resource->extension_public->execute = execute;
  surf_workstation_resource->extension_public->sleep = action_sleep;
  surf_workstation_resource->extension_public->get_state = get_state;
  surf_workstation_resource->extension_public->communicate = communicate;

  workstation_set = xbt_dict_new();

  xbt_assert0(maxmin_system, "surf_init has to be called first!");
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
void surf_workstation_resource_init_CLM03(const char *filename)
{
/*   int i ; */
/*   surf_resource_t resource =  NULL; */

  surf_workstation_resource_init_internal();
  surf_cpu_resource_init_Cas01(filename);
  surf_network_resource_init_CM02(filename);
  create_workstations();
  xbt_dynar_push(resource_list, &surf_workstation_resource);
/*   xbt_dynar_foreach(resource_list, i, resource) { */
/*     if(resource==surf_cpu_resource) { */
/*       xbt_dynar_remove_at(resource_list, i, NULL); */
/*       i--;  */
/*       continue; */
/*     } */
/*     if(resource==surf_network_resource) { */
/*       xbt_dynar_remove_at(resource_list, i, NULL); */
/*       i--;  */
/*       continue; */
/*     } */
/*   } */
}
