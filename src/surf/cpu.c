/* Authors: Arnaud Legrand                                                  */

/* This program is free software; you can redistribute it and/or modify it
   under the terms of the license (GNU LGPL) which comes with this package. */

#include "cpu_private.h"
#include "xbt/dict.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(cpu, surf,
				"Logging specific to the SURF CPU module");

surf_cpu_resource_t surf_cpu_resource = NULL;

static xbt_dict_t cpu_set = NULL;

static void cpu_free(void *CPU)
{
/*   cpu_t cpu = CPU; */
/* lmm_constraint_free(maxmin_system,cpu->constraint); */
/* Clean somewhere else ! */

  xbt_free(CPU);
}

/* power_scale is the basic power of the cpu when the cpu is
   completely available. power_initial is therefore expected to be
   comprised between 0.0 and 1.0, just as the values of power_trace.
   state_trace values mean SURF_CPU_ON if >0 and SURF_CPU_OFF
   otherwise.
*/
static cpu_t cpu_new(const char *name, xbt_maxmin_float_t power_scale,
		     xbt_maxmin_float_t power_initial,
		     tmgr_trace_t power_trace,
		     e_surf_cpu_state_t state_initial,
		     tmgr_trace_t state_trace)
{
  cpu_t cpu = xbt_new0(s_cpu_t, 1);

  cpu->resource = (surf_resource_t) surf_cpu_resource;
  cpu->name = name;
  cpu->power_scale = power_scale;
  cpu->power_current = power_initial;
/*   cpu->power_trace = power_trace; */
  if (power_trace)
    cpu->power_event =
	tmgr_history_add_trace(history, power_trace, 0.0, 0, cpu);

  cpu->state_current = state_initial;
/*   cpu->state_trace = state_trace; */
  if (state_trace)
    cpu->state_event =
	tmgr_history_add_trace(history, state_trace, 0.0, 0, cpu);

  cpu->constraint =
      lmm_constraint_new(maxmin_system, cpu,
			 cpu->power_current * cpu->power_scale);

  xbt_dict_set(cpu_set, name, cpu, cpu_free);

  return cpu;
}

/*  
   Semantic:  name       scale     initial     power     initial     state
                                    power      trace      state      trace
   
   Token:   TOKEN_WORD TOKEN_WORD TOKEN_WORD TOKEN_WORD TOKEN_WORD TOKEN_WORD
   Type:     string      float      float      string     ON/OFF     string
*/

static void parse_cpu(void)
{
  e_surf_token_t token;
  char *name = NULL;
  xbt_maxmin_float_t power_scale = 0.0;
  xbt_maxmin_float_t power_initial = 0.0;
  tmgr_trace_t power_trace = NULL;;
  e_surf_cpu_state_t state_initial = SURF_CPU_OFF;
  tmgr_trace_t state_trace = NULL;

  name = xbt_strdup(surf_parse_text);

  surf_parse_float(&power_scale);
  surf_parse_float(&power_initial);
  surf_parse_trace(&power_trace);

  token = surf_parse();		/* state_initial */
  xbt_assert1((token == TOKEN_WORD), "Parse error line %d", line_pos);
  if (strcmp(surf_parse_text, "ON") == 0)
    state_initial = SURF_CPU_ON;
  else if (strcmp(surf_parse_text, "OFF") == 0)
    state_initial = SURF_CPU_OFF;
  else {
    CRITICAL2("Invalid cpu state (line %d): %s neq ON or OFF\n", line_pos,
	      surf_parse_text);
    xbt_abort();
  }

  surf_parse_trace(&state_trace);

  cpu_new(name, power_scale, power_initial, power_trace, state_initial,
	  state_trace);
}

static void parse_file(const char *file)
{
  e_surf_token_t token;

  find_section(file, "CPU");

  while (1) {
    token = surf_parse();

    if (token == TOKEN_END_SECTION)
      break;
    if (token == TOKEN_NEWLINE)
      continue;

    if (token == TOKEN_WORD)
      parse_cpu();
    else {
      CRITICAL1("Parse error line %d\n", line_pos);
      xbt_abort();
    }
  }

  close_section("CPU");
}

static void *name_service(const char *name)
{
  void *cpu = NULL;

  xbt_dict_get(cpu_set, name, &cpu);

  return cpu;
}

static const char *get_resource_name(void *resource_id)
{
  return ((cpu_t) resource_id)->name;
}

static int resource_used(void *resource_id)
{
  return lmm_constraint_used(maxmin_system,
			     ((cpu_t) resource_id)->constraint);
}

static void action_free(surf_action_t action)
{
  surf_action_cpu_t Action = (surf_action_cpu_t) action;

  xbt_swag_remove(action, action->state_set);
  lmm_variable_free(maxmin_system, Action->variable);
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
  surf_action_change_state(action, state);
  return;
}

static xbt_heap_float_t share_resources(xbt_heap_float_t now)
{
  surf_action_cpu_t action = NULL;
  xbt_swag_t running_actions =
      surf_cpu_resource->common_public->states.running_action_set;
  xbt_maxmin_float_t min = -1;
  xbt_maxmin_float_t value = -1;
  lmm_solve(maxmin_system);

  action = xbt_swag_getFirst(running_actions);
  if (!action)
    return -1.0;
  value = lmm_variable_getvalue(action->variable);
  min = action->generic_action.remains / value;

  xbt_swag_foreach(action, running_actions) {
    value = action->generic_action.remains /
	lmm_variable_getvalue(action->variable);
    if (value < min)
      min = value;
  }

  return min;
}

static void update_actions_state(xbt_heap_float_t now,
				 xbt_heap_float_t delta)
{
  surf_action_cpu_t action = NULL;
  surf_action_cpu_t next_action = NULL;
  xbt_swag_t running_actions =
      surf_cpu_resource->common_public->states.running_action_set;
  xbt_swag_t failed_actions =
      surf_cpu_resource->common_public->states.failed_action_set;

  xbt_swag_foreach_safe(action, next_action, running_actions) {
    action->generic_action.remains -=
	lmm_variable_getvalue(action->variable) * delta;
/*     if(action->generic_action.remains<.00001) action->generic_action.remains=0; */
    if (action->generic_action.remains <= 0) {
      action_change_state((surf_action_t) action, SURF_ACTION_DONE);
    } else {			/* Need to check that none of the resource has failed */
      lmm_constraint_t cnst = NULL;
      int i = 0;
      cpu_t cpu = NULL;

      while ((cnst =
	      lmm_get_cnst_from_var(maxmin_system, action->variable,
				    i++))) {
	cpu = lmm_constraint_id(cnst);
	if (cpu->state_current == SURF_CPU_OFF) {
	  action_change_state((surf_action_t) action, SURF_ACTION_FAILED);
	  break;
	}
      }
    }
  }

  xbt_swag_foreach_safe(action, next_action, failed_actions) {
    lmm_variable_disable(maxmin_system, action->variable);
  }

  return;
}

static void update_resource_state(void *id,
				  tmgr_trace_event_t event_type,
				  xbt_maxmin_float_t value)
{
  cpu_t cpu = id;

/*   printf("[" XBT_HEAP_FLOAT_T "] Asking to update CPU \"%s\" with value " */
/* 	 XBT_MAXMIN_FLOAT_T " for event %p\n", surf_get_clock(), cpu->name, */
/* 	 value, event_type); */

  if (event_type == cpu->power_event) {
    cpu->power_current = value;
    lmm_update_constraint_bound(maxmin_system,cpu->constraint,
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

static surf_action_t execute(void *cpu, xbt_maxmin_float_t size)
{
  surf_action_cpu_t action = NULL;
  cpu_t CPU = cpu;

  action = xbt_new0(s_surf_action_cpu_t, 1);

  action->generic_action.cost = size;
  action->generic_action.remains = size;
  action->generic_action.start = -1.0;
  action->generic_action.finish = -1.0;
/*   action->generic_action.callback = cpu; */
  action->generic_action.callback = NULL;
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
  lmm_expand(maxmin_system, ((cpu_t) cpu)->constraint, action->variable,
	     1.0);

  return (surf_action_t) action;
}

static e_surf_cpu_state_t get_state(void *cpu)
{
  return SURF_CPU_OFF;
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
/*   surf_cpu_resource->extension_private = xbt_new0(s_surf_cpu_resource_extension_private_t,1); */
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

  surf_cpu_resource->common_private->resource_used = resource_used;
  surf_cpu_resource->common_private->share_resources = share_resources;
  surf_cpu_resource->common_private->update_actions_state =
      update_actions_state;
  surf_cpu_resource->common_private->update_resource_state =
      update_resource_state;
  surf_cpu_resource->common_private->finalize = finalize;

  surf_cpu_resource->extension_public->execute = execute;
  surf_cpu_resource->extension_public->get_state = get_state;

  cpu_set = xbt_dict_new();

  xbt_assert0(maxmin_system, "surf_init has to be called first!");
}

void surf_cpu_resource_init(const char *filename)
{
  surf_cpu_resource_init_internal();
  parse_file(filename);
  xbt_dynar_push(resource_list, &surf_cpu_resource);
}
