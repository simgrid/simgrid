/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "cpu_private.h"
#include "xbt/dict.h"

surf_cpu_resource_t surf_cpu_resource = NULL;
static xbt_dict_t cpu_set = NULL;
static lmm_system_t sys = NULL;

typedef struct cpu {
  const char *name;
  xbt_maxmin_float_t power_scale;
  xbt_maxmin_float_t current_power;
  tmgr_trace_t power_trace;
  e_surf_action_state_t current_state;
  tmgr_trace_t state_trace;
  lmm_constraint_t constraint;
} s_cpu_t, *cpu_t;

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

  cpu->name = name;
  cpu->power_scale = power_scale;
  cpu->current_power = initial_power;
  cpu->power_trace = power_trace;
  cpu->current_state = initial_state;
  cpu->state_trace = state_trace;
  cpu->constraint = lmm_constraint_new(sys, cpu, cpu->current_power);

  xbt_dict_set(cpu_set, name, cpu, NULL);

  return cpu;
}

static void parse_file(const char *file)
{
  new_cpu("Cpu A", 200.0, 1.0, NULL, SURF_CPU_ON, NULL);
  new_cpu("Cpu B", 100.0, 1.0, NULL, SURF_CPU_ON, NULL);
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

static xbt_heap_float_t share_resources(void)
{
  return -1.0;
}

static void solve(xbt_heap_float_t date)
{
  return;
}

static surf_action_t execute(void *cpu, xbt_maxmin_float_t size)
{
  return NULL;
}

static e_surf_cpu_state_t get_state(void *cpu)
{
  return SURF_CPU_OFF;
}


surf_cpu_resource_t surf_cpu_resource_init(void)
{
  surf_cpu_resource = xbt_new0(s_surf_cpu_resource_t,1);

  surf_cpu_resource->resource.parse_file = parse_file;
  surf_cpu_resource->resource.name_service = name_service;
  surf_cpu_resource->resource.get_resource_name = get_resource_name;
  surf_cpu_resource->resource.action_get_state=surf_action_get_state;
  surf_cpu_resource->resource.action_free = action_free;
  surf_cpu_resource->resource.action_cancel = action_cancel;
  surf_cpu_resource->resource.action_recycle = action_recycle;
  surf_cpu_resource->resource.action_change_state = action_change_state;
  surf_cpu_resource->resource.share_resources = share_resources;
  surf_cpu_resource->resource.solve = solve;

  surf_cpu_resource->execute = execute;
  surf_cpu_resource->get_state = get_state;

  cpu_set = xbt_dict_new();

  sys = lmm_system_new();

  return surf_cpu_resource;
}
