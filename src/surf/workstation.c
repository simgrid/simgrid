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

static int parallel_action_free(surf_action_t action)
{
  action->using--;
  if(!action->using) {
    xbt_swag_remove(action, action->state_set);
    if(((surf_action_parallel_task_CSL05_t)action)->variable)
      lmm_variable_free(maxmin_system, ((surf_action_parallel_task_CSL05_t)action)->variable);
    free(action);
    return 1;
  }
  return 0;
}

static void parallel_action_use(surf_action_t action)
{
  action->using++;
}

static int action_free(surf_action_t action)
{
  if(action->resource_type==(surf_resource_t)surf_network_resource) 
    return surf_network_resource->common_public->action_free(action);
  else if(action->resource_type==(surf_resource_t)surf_cpu_resource) 
    return surf_cpu_resource->common_public->action_free(action);
  else if(action->resource_type==(surf_resource_t)surf_workstation_resource)
    return parallel_action_free(action);
  else DIE_IMPOSSIBLE;
  return 0;
}

static void action_use(surf_action_t action)
{
  if(action->resource_type==(surf_resource_t)surf_network_resource) 
    surf_network_resource->common_public->action_use(action);
  else if(action->resource_type==(surf_resource_t)surf_cpu_resource) 
    surf_cpu_resource->common_public->action_use(action);
  else if(action->resource_type==(surf_resource_t)surf_workstation_resource)
    return parallel_action_use(action);
  else DIE_IMPOSSIBLE;
  return;
}

static void action_cancel(surf_action_t action)
{
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
  if(action->resource_type==(surf_resource_t)surf_network_resource) 
    surf_network_resource->common_public->action_change_state(action,state);
  else if(action->resource_type==(surf_resource_t)surf_cpu_resource) 
    surf_cpu_resource->common_public->action_change_state(action,state);
  else if(action->resource_type==(surf_resource_t)surf_workstation_resource)
    surf_action_change_state(action, state);
  else DIE_IMPOSSIBLE;
  return;
}

static double share_resources(double now)
{
  s_surf_action_parallel_task_CSL05_t action;
  return generic_maxmin_share_resources(surf_workstation_resource->common_public->
					states.running_action_set,
					xbt_swag_offset(action, variable));
}

static void update_actions_state(double now, double delta)
{
  surf_action_parallel_task_CSL05_t action = NULL;
  surf_action_parallel_task_CSL05_t next_action = NULL;
  xbt_swag_t running_actions =
      surf_workstation_resource->common_public->states.running_action_set;
  xbt_swag_t failed_actions =
      surf_workstation_resource->common_public->states.failed_action_set;

  xbt_swag_foreach_safe(action, next_action, running_actions) {
    surf_double_update(&(action->generic_action.remains),
	lmm_variable_getvalue(action->variable) * delta);
    if (action->generic_action.max_duration != NO_MAX_DURATION)
      surf_double_update(&(action->generic_action.max_duration), delta);
    if ((action->generic_action.remains <= 0) && 
	(lmm_get_variable_weight(action->variable)>0)) {
      action->generic_action.finish = surf_get_clock();
      action_change_state((surf_action_t) action, SURF_ACTION_DONE);
    } else if ((action->generic_action.max_duration != NO_MAX_DURATION) &&
	       (action->generic_action.max_duration <= 0)) {
      action->generic_action.finish = surf_get_clock();
      action_change_state((surf_action_t) action, SURF_ACTION_DONE);
    } else {			/* Need to check that none of the resource has failed */
      lmm_constraint_t cnst = NULL;
      int i = 0;
      surf_resource_t resource = NULL;

      while ((cnst =
	      lmm_get_cnst_from_var(maxmin_system, action->variable,
				    i++))) {
	resource = (surf_resource_t) lmm_constraint_id(cnst);
	if(resource== (surf_resource_t) surf_cpu_resource) {
	  cpu_Cas01_t cpu = lmm_constraint_id(cnst);
	  if (cpu->state_current == SURF_CPU_OFF) {
	    action->generic_action.finish = surf_get_clock();
	    action_change_state((surf_action_t) action, SURF_ACTION_FAILED);
	    break;
	  }
	} else if (resource== (surf_resource_t) surf_network_resource) {
	  network_link_CM02_t nw_link = lmm_constraint_id(cnst);

	  if (nw_link->state_current == SURF_NETWORK_LINK_OFF) {
	    action->generic_action.finish = surf_get_clock();
	    action_change_state((surf_action_t) action, SURF_ACTION_FAILED);
	    break;
	  }
	} 
      }
    }
  }

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

static void action_set_max_duration(surf_action_t action, double duration)
{
  if(action->resource_type==(surf_resource_t)surf_network_resource)
    surf_network_resource->common_public->set_max_duration(action,duration);
  else if(action->resource_type==(surf_resource_t)surf_cpu_resource) 
    surf_cpu_resource->common_public->set_max_duration(action,duration);
  else  DIE_IMPOSSIBLE;
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

static surf_action_t execute_parallel_task (int workstation_nb,
					    void **workstation_list,
					    double *computation_amount,
					    double *communication_amount,
					    double amount,
					    double rate)
{
  surf_action_parallel_task_CSL05_t action = NULL;
  int i, j, k;
  xbt_dict_t network_link_set = xbt_dict_new();
  xbt_dict_cursor_t cursor = NULL;
  char *name = NULL;
  int nb_link = 0;
  network_link_CM02_t link;

  /* Compute the number of affected resources... */
  for(i=0; i< workstation_nb; i++) {
    for(j=0; j< workstation_nb; j++) {
      network_card_CM02_t card_src = ((workstation_CLM03_t*)workstation_list)[i]->network_card;
      network_card_CM02_t card_dst = ((workstation_CLM03_t*)workstation_list)[j]->network_card;
      int route_size = ROUTE_SIZE(card_src->id, card_dst->id);
      network_link_CM02_t *route = ROUTE(card_src->id, card_dst->id);
      
      if(communication_amount[i*workstation_nb+j]>=0)
	for(k=0; k< route_size; k++) {
	  xbt_dict_set(network_link_set, route[k]->name, route[k], NULL);
	}
    }
  }
 
  xbt_dict_foreach(network_link_set, cursor, name, link) {
    nb_link++;
  }
  
  action = xbt_new0(s_surf_action_parallel_task_CSL05_t, 1);
  action->generic_action.using = 1;
  action->generic_action.cost = amount;
  action->generic_action.remains = amount;
  action->generic_action.max_duration = NO_MAX_DURATION;
  action->generic_action.start = -1.0;
  action->generic_action.finish = -1.0;
  action->generic_action.resource_type =
      (surf_resource_t) surf_workstation_resource;
  action->suspended = 0;  /* Should be useless because of the
			     calloc but it seems to help valgrind... */
  action->generic_action.state_set =
      surf_workstation_resource->common_public->states.running_action_set;

  xbt_swag_insert(action, action->generic_action.state_set);
  action->rate = rate;

  if(action->rate>0)
    action->variable = lmm_variable_new(maxmin_system, action, 1.0, -1.0,
					workstation_nb + nb_link);
  else   
    action->variable = lmm_variable_new(maxmin_system, action, 1.0, action->rate,
					workstation_nb + nb_link);

  if(nb_link + workstation_nb == 0)
    action_change_state((surf_action_t) action, SURF_ACTION_DONE);

  for (i = 0; i<workstation_nb; i++)
    lmm_expand(maxmin_system, ((cpu_Cas01_t) ((workstation_CLM03_t) workstation_list[i])->cpu)->constraint, 
	       action->variable, computation_amount[i]);

  for (i=0; i<workstation_nb; i++) {
    for(j=0; j< workstation_nb; j++) {
      network_card_CM02_t card_src = ((workstation_CLM03_t*)workstation_list)[i]->network_card;
      network_card_CM02_t card_dst = ((workstation_CLM03_t*)workstation_list)[j]->network_card;
      int route_size = ROUTE_SIZE(card_src->id, card_dst->id);
      network_link_CM02_t *route = ROUTE(card_src->id, card_dst->id);
      
      for(k=0; k< route_size; k++) {
	if(communication_amount[i*workstation_nb+j]>=0) {
	  lmm_expand_add(maxmin_system, route[k]->constraint, 
		       action->variable, communication_amount[i*workstation_nb+j]);
	}
      }
    }
  }
  
  return (surf_action_t) action;
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
  surf_workstation_resource->common_public->action_use = action_use;
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
  surf_workstation_resource->common_public->set_max_duration = action_set_max_duration;

  surf_workstation_resource->extension_public->execute = execute;
  surf_workstation_resource->extension_public->sleep = action_sleep;
  surf_workstation_resource->extension_public->get_state = get_state;
  surf_workstation_resource->extension_public->communicate = communicate;
  surf_workstation_resource->extension_public->execute_parallel_task = 
    execute_parallel_task;

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
