/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "cpu_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(cpu, surf,
				"Logging specific to the SURF CPU module");

surf_cpu_resource_t surf_cpu_resource = NULL;

xbt_dict_t cpu_set = NULL;

static void cpu_free(void *cpu)
{
  xbt_free(((cpu_Cas01_t)cpu)->name);
  xbt_free(cpu);
}

static cpu_Cas01_t cpu_new(char *name, double power_scale,
		     double power_initial,
		     tmgr_trace_t power_trace,
		     e_surf_cpu_state_t state_initial,
		     tmgr_trace_t state_trace)
{
  cpu_Cas01_t cpu = xbt_new0(s_cpu_Cas01_t, 1);

  cpu->resource = (surf_resource_t) surf_cpu_resource;
  cpu->name = name;
  cpu->power_scale = power_scale;
  xbt_assert0(cpu->power_scale>0,"Power has to be >0");
  cpu->power_current = power_initial;
  if (power_trace)
    cpu->power_event =
	tmgr_history_add_trace(history, power_trace, 0.0, 0, cpu);

  cpu->state_current = state_initial;
  if (state_trace)
    cpu->state_event =
	tmgr_history_add_trace(history, state_trace, 0.0, 0, cpu);

  cpu->constraint =
      lmm_constraint_new(maxmin_system, cpu,
			 cpu->power_current * cpu->power_scale);

  xbt_dict_set(cpu_set, name, cpu, cpu_free);

  return cpu;
}

static void parse_cpu(void)
{
  char *name = NULL;
  double power_scale = 0.0;
  double power_initial = 0.0;
  tmgr_trace_t power_trace = NULL;
  e_surf_cpu_state_t state_initial = SURF_CPU_OFF;
  tmgr_trace_t state_trace = NULL;

  name = xbt_strdup(A_cpu_name);
  surf_parse_get_double(&power_scale,A_cpu_power);
  surf_parse_get_double(&power_initial,A_cpu_availability);
  surf_parse_get_trace(&power_trace,A_cpu_availability_file);

  xbt_assert0((A_cpu_state==A_cpu_state_ON)||(A_cpu_state==A_cpu_state_OFF),
	      "Invalid state")
  if (A_cpu_state==A_cpu_state_ON) state_initial = SURF_CPU_ON;
  if (A_cpu_state==A_cpu_state_OFF) state_initial = SURF_CPU_OFF;
  surf_parse_get_trace(&state_trace,A_cpu_state_file);

  cpu_new(name, power_scale, power_initial, power_trace, state_initial,
	  state_trace);
}

static void parse_file(const char *file)
{
  surf_parse_reset_parser();
  ETag_cpu_fun=parse_cpu;
  surf_parse_open(file);
  xbt_assert1((!surf_parse()),"Parse error in %s",file);
  surf_parse_close();
}

static void *name_service(const char *name)
{
  void *cpu = NULL;

  xbt_dict_get(cpu_set, name, &cpu);

  return cpu;
}

static const char *get_resource_name(void *resource_id)
{
  return ((cpu_Cas01_t) resource_id)->name;
}

static int resource_used(void *resource_id)
{
  return lmm_constraint_used(maxmin_system,
			     ((cpu_Cas01_t) resource_id)->constraint);
}

static void action_free(surf_action_t action)
{
  xbt_swag_remove(action, action->state_set);
  if(((surf_action_cpu_Cas01_t)action)->variable)
    lmm_variable_free(maxmin_system, ((surf_action_cpu_Cas01_t)action)->variable);
  xbt_free(action);

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
  if((state==SURF_ACTION_DONE) || (state==SURF_ACTION_FAILED))
    if(((surf_action_cpu_Cas01_t)action)->variable) {
      lmm_variable_disable(maxmin_system, ((surf_action_cpu_Cas01_t)action)->variable);
      ((surf_action_cpu_Cas01_t)action)->variable = NULL;
    }

  surf_action_change_state(action, state);
  return;
}

static double share_resources(double now)
{
  s_surf_action_cpu_Cas01_t action;
  return generic_maxmin_share_resources(surf_cpu_resource->common_public->
					states.running_action_set,
					xbt_swag_offset(action, variable));
}

static void update_actions_state(double now, double delta)
{
  surf_action_cpu_Cas01_t action = NULL;
  surf_action_cpu_Cas01_t next_action = NULL;
  xbt_swag_t running_actions =
      surf_cpu_resource->common_public->states.running_action_set;
  xbt_swag_t failed_actions =
      surf_cpu_resource->common_public->states.failed_action_set;

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
      cpu_Cas01_t cpu = NULL;

      while ((cnst =
	      lmm_get_cnst_from_var(maxmin_system, action->variable,
				    i++))) {
	cpu = lmm_constraint_id(cnst);
	if (cpu->state_current == SURF_CPU_OFF) {
	  action->generic_action.finish = surf_get_clock();
	  action_change_state((surf_action_t) action, SURF_ACTION_FAILED);
	  break;
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
  cpu_Cas01_t cpu = id;

  if (event_type == cpu->power_event) {
    cpu->power_current = value;
    lmm_update_constraint_bound(maxmin_system, cpu->constraint,
				cpu->power_current * cpu->power_scale);
  } else if (event_type == cpu->state_event) {
    if (value > 0)
      cpu->state_current = SURF_CPU_ON;
    else
      cpu->state_current = SURF_CPU_OFF;
  } else {
    CRITICAL0("Unknown event ! \n");
    xbt_abort();
  }

  return;
}

static surf_action_t execute(void *cpu, double size)
{
  surf_action_cpu_Cas01_t action = NULL;
  cpu_Cas01_t CPU = cpu;

  action = xbt_new0(s_surf_action_cpu_Cas01_t, 1);

  action->generic_action.cost = size;
  action->generic_action.remains = size;
  action->generic_action.max_duration = NO_MAX_DURATION;
  action->generic_action.start = surf_get_clock();
  action->generic_action.finish = -1.0;
  action->generic_action.resource_type =
      (surf_resource_t) surf_cpu_resource;

  if (CPU->state_current == SURF_CPU_ON)
    action->generic_action.state_set =
	surf_cpu_resource->common_public->states.running_action_set;
  else
    action->generic_action.state_set =
	surf_cpu_resource->common_public->states.failed_action_set;
  xbt_swag_insert(action, action->generic_action.state_set);

  action->variable = lmm_variable_new(maxmin_system, action, 1.0, -1.0, 1);
  lmm_expand(maxmin_system, CPU->constraint, action->variable,
	     1.0);

  return (surf_action_t) action;
}

static surf_action_t action_sleep(void *cpu, double duration)
{
  surf_action_cpu_Cas01_t action = NULL;

  action = (surf_action_cpu_Cas01_t) execute(cpu, 1.0);
  action->generic_action.max_duration = duration;
  lmm_update_variable_weight(maxmin_system, action->variable, 0.0);

  return (surf_action_t) action;
}

static void action_suspend(surf_action_t action)
{
  lmm_update_variable_weight(maxmin_system,
			     ((surf_action_cpu_Cas01_t) action)->variable, 0.0);
}

static void action_resume(surf_action_t action)
{
  lmm_update_variable_weight(maxmin_system,
			     ((surf_action_cpu_Cas01_t) action)->variable, 1.0);
}

static int action_is_suspended(surf_action_t action)
{
  return (lmm_get_variable_weight(((surf_action_cpu_Cas01_t) action)->variable) == 0.0);
}

static e_surf_cpu_state_t get_state(void *cpu)
{
  return ((cpu_Cas01_t) cpu)->state_current;
}

static void finalize(void)
{
  xbt_dict_free(&cpu_set);
  xbt_swag_free(surf_cpu_resource->common_public->states.ready_action_set);
  xbt_swag_free(surf_cpu_resource->common_public->states.
		running_action_set);
  xbt_swag_free(surf_cpu_resource->common_public->states.
		failed_action_set);
  xbt_swag_free(surf_cpu_resource->common_public->states.done_action_set);
  xbt_free(surf_cpu_resource->common_public);
  xbt_free(surf_cpu_resource->common_private);
  xbt_free(surf_cpu_resource->extension_public);

  xbt_free(surf_cpu_resource);
  surf_cpu_resource = NULL;
}

static void surf_cpu_resource_init_internal(void)
{
  s_surf_action_t action;

  surf_cpu_resource = xbt_new0(s_surf_cpu_resource_t, 1);

  surf_cpu_resource->common_private =
      xbt_new0(s_surf_resource_private_t, 1);
  surf_cpu_resource->common_public = xbt_new0(s_surf_resource_public_t, 1);

  surf_cpu_resource->extension_public =
      xbt_new0(s_surf_cpu_resource_extension_public_t, 1);

  surf_cpu_resource->common_public->states.ready_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_cpu_resource->common_public->states.running_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_cpu_resource->common_public->states.failed_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));
  surf_cpu_resource->common_public->states.done_action_set =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));

  surf_cpu_resource->common_public->name_service = name_service;
  surf_cpu_resource->common_public->get_resource_name = get_resource_name;
  surf_cpu_resource->common_public->action_get_state =
      surf_action_get_state;
  surf_cpu_resource->common_public->action_free = action_free;
  surf_cpu_resource->common_public->action_cancel = action_cancel;
  surf_cpu_resource->common_public->action_recycle = action_recycle;
  surf_cpu_resource->common_public->action_change_state =
      action_change_state;
  surf_cpu_resource->common_public->action_set_data = surf_action_set_data;
  surf_cpu_resource->common_public->name = "CPU";

  surf_cpu_resource->common_private->resource_used = resource_used;
  surf_cpu_resource->common_private->share_resources = share_resources;
  surf_cpu_resource->common_private->update_actions_state =
      update_actions_state;
  surf_cpu_resource->common_private->update_resource_state =
      update_resource_state;
  surf_cpu_resource->common_private->finalize = finalize;

  surf_cpu_resource->common_public->suspend = action_suspend;
  surf_cpu_resource->common_public->resume = action_resume;
  surf_cpu_resource->common_public->is_suspended = action_is_suspended;

  surf_cpu_resource->extension_public->execute = execute;
  surf_cpu_resource->extension_public->sleep = action_sleep;

  surf_cpu_resource->extension_public->get_state = get_state;

  cpu_set = xbt_dict_new();

  xbt_assert0(maxmin_system, "surf_init has to be called first!");
}

/*********************************************************************/
/* Basic sharing model for CPU: that is where all this started... ;) */
/*********************************************************************/
/* @InProceedings{casanova01simgrid, */
/*   author =       "H. Casanova", */
/*   booktitle =    "Proceedings of the IEEE Symposium on Cluster Computing */
/*                  and the Grid (CCGrid'01)", */
/*   publisher =    "IEEE Computer Society", */
/*   title =        "Simgrid: {A} Toolkit for the Simulation of Application */
/*                  Scheduling", */
/*   year =         "2001", */
/*   month =        may, */
/*   note =         "Available at */
/*                  \url{http://grail.sdsc.edu/papers/simgrid_ccgrid01.ps.gz}." */
/* } */
void surf_cpu_resource_init_Cas01(const char *filename)
{
  if (surf_cpu_resource)
    return;
  surf_cpu_resource_init_internal();
  parse_file(filename);
  xbt_dynar_push(resource_list, &surf_cpu_resource);
}
