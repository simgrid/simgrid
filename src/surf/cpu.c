/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "cpu_private.h"
#include "xbt/dict.h"

surf_cpu_resource_t surf_cpu_resource = NULL;

static xbt_dict_t cpu_set = NULL;
static lmm_system_t sys = NULL;

/* power_scale is the basic power of the cpu when the cpu is
   completely available. initial_power is therefore expected to be
   comprised between 0.0 and 1.0, just as the values of power_trace.
   state_trace values mean SURF_CPU_ON if >0 and SURF_CPU_OFF
   otherwise.
*/

static void *new_cpu(const char *name, xbt_maxmin_float_t power_scale,
		      xbt_maxmin_float_t initial_power, tmgr_trace_t power_trace,
		      e_surf_cpu_state_t initial_state, tmgr_trace_t state_trace)
{
  cpu_t cpu = xbt_new0(s_cpu_t,1);

  cpu->resource = (surf_resource_t) surf_cpu_resource;
  cpu->name = name;
  cpu->power_scale = power_scale;
  cpu->current_power = initial_power;
/*   cpu->power_trace = power_trace; */
  if(power_trace) 
    cpu->power_event = tmgr_history_add_trace(history, power_trace, 0.0, 0, cpu);

  cpu->current_state = initial_state;
/*   cpu->state_trace = state_trace; */
  if(state_trace) 
    cpu->state_event = tmgr_history_add_trace(history, state_trace, 0.0, 0, cpu);

  cpu->constraint = lmm_constraint_new(sys, cpu, cpu->current_power * cpu->power_scale);

  xbt_dict_set(cpu_set, name, cpu, NULL);

  return cpu;
}

static void parse_file(const char *file)
{
  tmgr_trace_t trace_A = tmgr_trace_new("trace_A.txt");
  tmgr_trace_t trace_B = tmgr_trace_new("trace_B.txt");

  new_cpu("Cpu A", 20.0, 1.0, trace_A, SURF_CPU_ON, NULL);
  new_cpu("Cpu B", 100.0, 1.0, trace_B, SURF_CPU_ON, NULL);
}

static void *name_service(const char *name)
{
  void *cpu=NULL;
  
  xbt_dict_get(cpu_set, name, &cpu);

  return cpu;
}

static const char *get_resource_name(void *resource_id)
{
  return ((cpu_t) resource_id)->name;
}

static int resource_used(void *resource_id)
{
  return lmm_constraint_used(sys, ((cpu_t)resource_id)->constraint);
}

static surf_action_t action_new(void *callback)
{
  return NULL;
}

static e_surf_action_state_t action_get_state(surf_action_t action)
{
  return SURF_ACTION_NOT_IN_THE_SYSTEM;
}

static void action_free(surf_action_t * action)
{
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

static void action_change_state(surf_action_t action, e_surf_action_state_t state)
{
  surf_action_change_state(action, state);
  return;
}

static xbt_heap_float_t share_resources()
{
  surf_action_cpu_t action = NULL;
  xbt_swag_t running_actions= surf_cpu_resource->common_public->states.running_action_set;
  xbt_maxmin_float_t min = -1;
  xbt_maxmin_float_t value = -1;
  lmm_solve(sys);  

  action = xbt_swag_getFirst(running_actions);
  if(!action) return 0.0;
  value = lmm_variable_getvalue(action->variable);
  min = action->generic_action.remains / value ;

  xbt_swag_foreach(action,running_actions) {
    value = action->generic_action.remains / 
      lmm_variable_getvalue(action->variable);
    if(value<min) min=value;
  }

  return min;
}

static void update_actions_state(xbt_heap_float_t now,
				 xbt_heap_float_t delta)
{
  surf_action_cpu_t action = NULL;
  surf_action_cpu_t next_action = NULL;
  xbt_swag_t running_actions= surf_cpu_resource->common_public->states.running_action_set;

  xbt_swag_foreach_safe(action, next_action, running_actions) {
    action->generic_action.remains -= 
      lmm_variable_getvalue(action->variable)*delta;
/*     if(action->generic_action.remains<.00001) action->generic_action.remains=0; */
    if(action->generic_action.remains<=0) {
      action_change_state((surf_action_t)action, SURF_ACTION_DONE);
    } /* else if(host_failed..) */
  }

  return;
}

static void update_resource_state(void *id,
				  tmgr_trace_event_t event_type, 
				  xbt_maxmin_float_t value)
{
  cpu_t cpu = id;
  
  printf("Asking to update \"%s\" with value " XBT_MAXMIN_FLOAT_T " for event %p\n",
	 cpu->name, value, event_type);
  
  if(event_type==cpu->power_event) {
    cpu->current_power = value;
    lmm_update_constraint_bound(cpu->constraint,cpu->current_power * cpu->power_scale);
  } else if (event_type==cpu->state_event) {
    if(value>0) cpu->current_state = SURF_CPU_ON;
    else cpu->current_state = SURF_CPU_OFF;
  } else abort();

  return;
}

static surf_action_t execute(void *cpu, xbt_maxmin_float_t size)
{
  lmm_variable_t var;
  surf_action_cpu_t action = NULL;

  action=xbt_new0(s_surf_action_cpu_t,1);

  action->generic_action.cost=size;
  action->generic_action.remains=size;  
  action->generic_action.start=-1.0;
  action->generic_action.finish=-1.0;
  action->generic_action.callback=cpu;
  action->generic_action.resource_type=(surf_resource_t)surf_cpu_resource;

  action->generic_action.state_set=surf_cpu_resource->common_public->states.running_action_set;
  xbt_swag_insert(action,action->generic_action.state_set);

  action->variable = lmm_variable_new(sys, action, 1.0, -1.0, 1);
  lmm_expand(sys, ((cpu_t)cpu)->constraint, action->variable, 1.0);

  return (surf_action_t) action;
}

static e_surf_cpu_state_t get_state(void *cpu)
{
  return SURF_CPU_OFF;
}

static void surf_cpu_resource_init_internal(void)
{
  s_surf_action_t action;

  surf_cpu_resource = xbt_new0(s_surf_cpu_resource_t,1);
  
  surf_cpu_resource->common_private = xbt_new0(s_surf_resource_private_t,1);
  surf_cpu_resource->common_public = xbt_new0(s_surf_resource_public_t,1);
/*   surf_cpu_resource->extension_private = xbt_new0(s_surf_cpu_resource_extension_private_t,1); */
  surf_cpu_resource->extension_public = xbt_new0(s_surf_cpu_resource_extension_public_t,1);

  surf_cpu_resource->common_public->states.ready_action_set=
    xbt_swag_new(xbt_swag_offset(action,state_hookup));
  surf_cpu_resource->common_public->states.running_action_set=
    xbt_swag_new(xbt_swag_offset(action,state_hookup));
  surf_cpu_resource->common_public->states.failed_action_set=
    xbt_swag_new(xbt_swag_offset(action,state_hookup));
  surf_cpu_resource->common_public->states.done_action_set=
    xbt_swag_new(xbt_swag_offset(action,state_hookup));

  surf_cpu_resource->common_public->name_service = name_service;
  surf_cpu_resource->common_public->get_resource_name = get_resource_name;
  surf_cpu_resource->common_public->resource_used = resource_used;
  surf_cpu_resource->common_public->action_get_state=surf_action_get_state;
  surf_cpu_resource->common_public->action_free = action_free;
  surf_cpu_resource->common_public->action_cancel = action_cancel;
  surf_cpu_resource->common_public->action_recycle = action_recycle;
  surf_cpu_resource->common_public->action_change_state = action_change_state;
  surf_cpu_resource->common_private->share_resources = share_resources;
  surf_cpu_resource->common_private->update_actions_state = update_actions_state;
  surf_cpu_resource->common_private->update_resource_state = update_resource_state;

  surf_cpu_resource->extension_public->execute = execute;
  surf_cpu_resource->extension_public->get_state = get_state;

  cpu_set = xbt_dict_new();

  sys = lmm_system_new();
}

void surf_cpu_resource_init(const char* filename)
{
  surf_cpu_resource_init_internal();
  parse_file(filename);
  xbt_dynar_push(resource_list, &surf_cpu_resource);
}
