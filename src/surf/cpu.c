/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "cpu_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_cpu, surf,
                                "Logging specific to the SURF CPU module");

surf_cpu_model_t surf_cpu_model = NULL;
lmm_system_t cpu_maxmin_system = NULL;


static xbt_swag_t running_action_set_that_does_not_need_being_checked = NULL;

static void cpu_free(void *cpu)
{
  free(((cpu_Cas01_t) cpu)->name);
  xbt_dict_free(&(((cpu_Cas01_t) cpu)->properties));
  free(cpu);
}

static cpu_Cas01_t cpu_new(char *name, double power_scale,
                           double power_initial,
                           tmgr_trace_t power_trace,
                           e_surf_cpu_state_t state_initial,
                           tmgr_trace_t state_trace,
                           xbt_dict_t cpu_properties)
{
  cpu_Cas01_t cpu = xbt_new0(s_cpu_Cas01_t, 1);
  xbt_assert1(!surf_model_resource_by_name(surf_cpu_model, name),
              "Host '%s' declared several times in the platform file", name);
  cpu->model = (surf_model_t) surf_cpu_model;
  cpu->name = name;
  cpu->power_scale = power_scale;
  xbt_assert0(cpu->power_scale > 0, "Power has to be >0");
  cpu->power_current = power_initial;
  if (power_trace)
    cpu->power_event =
      tmgr_history_add_trace(history, power_trace, 0.0, 0, cpu);

  cpu->state_current = state_initial;
  if (state_trace)
    cpu->state_event =
      tmgr_history_add_trace(history, state_trace, 0.0, 0, cpu);

  cpu->constraint =
    lmm_constraint_new(cpu_maxmin_system, cpu,
                       cpu->power_current * cpu->power_scale);

  /*add the property set */
  cpu->properties = cpu_properties;

  current_property_set = cpu_properties;

  xbt_dict_set(surf_model_resource_set(surf_cpu_model), name, cpu, cpu_free);

  return cpu;
}


static void parse_cpu_init(void)
{
  double power_scale = 0.0;
  double power_initial = 0.0;
  tmgr_trace_t power_trace = NULL;
  e_surf_cpu_state_t state_initial = SURF_CPU_OFF;
  tmgr_trace_t state_trace = NULL;

  power_scale = get_cpu_power(A_surfxml_host_power);
  surf_parse_get_double(&power_initial, A_surfxml_host_availability);
  surf_parse_get_trace(&power_trace, A_surfxml_host_availability_file);

  xbt_assert0((A_surfxml_host_state == A_surfxml_host_state_ON) ||
              (A_surfxml_host_state == A_surfxml_host_state_OFF),
              "Invalid state");
  if (A_surfxml_host_state == A_surfxml_host_state_ON)
    state_initial = SURF_CPU_ON;
  if (A_surfxml_host_state == A_surfxml_host_state_OFF)
    state_initial = SURF_CPU_OFF;
  surf_parse_get_trace(&state_trace, A_surfxml_host_state_file);

  current_property_set = xbt_dict_new();
  cpu_new(xbt_strdup(A_surfxml_host_id), power_scale, power_initial,
          power_trace, state_initial, state_trace, current_property_set);

}

static void add_traces_cpu(void)
{
  xbt_dict_cursor_t cursor = NULL;
  char *trace_name, *elm;

  static int called = 0;

  if (called)
    return;
  called = 1;


  /* connect all traces relative to hosts */
  xbt_dict_foreach(trace_connect_list_host_avail, cursor, trace_name, elm) {
    tmgr_trace_t trace = xbt_dict_get_or_null(traces_set_list, trace_name);
    cpu_Cas01_t host = surf_model_resource_by_name((surf_model_t)surf_cpu_model, elm);

    xbt_assert1(host, "Host %s undefined", elm);
    xbt_assert1(trace, "Trace %s undefined", trace_name);

    host->state_event = tmgr_history_add_trace(history, trace, 0.0, 0, host);
  }

  xbt_dict_foreach(trace_connect_list_power, cursor, trace_name, elm) {
    tmgr_trace_t trace = xbt_dict_get_or_null(traces_set_list, trace_name);
    cpu_Cas01_t host = surf_model_resource_by_name((surf_model_t)surf_cpu_model, elm);

    xbt_assert1(host, "Host %s undefined", elm);
    xbt_assert1(trace, "Trace %s undefined", trace_name);

    host->power_event = tmgr_history_add_trace(history, trace, 0.0, 0, host);
  }
}

static void define_callbacks(const char *file)
{
  surf_parse_reset_parser();
  surfxml_add_callback(STag_surfxml_host_cb_list, parse_cpu_init);
}

static const char *get_resource_name(void *resource_id)
{
  return ((cpu_Cas01_t) resource_id)->name;
}

static int resource_used(void *resource_id)
{
  return lmm_constraint_used(cpu_maxmin_system,
                             ((cpu_Cas01_t) resource_id)->constraint);
}

static int action_free(surf_action_t action)
{
  action->refcount--;
  if (!action->refcount) {
    xbt_swag_remove(action, action->state_set);
    if (((surf_action_cpu_Cas01_t) action)->variable)
      lmm_variable_free(cpu_maxmin_system,
                        ((surf_action_cpu_Cas01_t) action)->variable);
    free(action);
    return 1;
  }
  return 0;
}

static void action_use(surf_action_t action)
{
  action->refcount++;
}

static void action_cancel(surf_action_t action)
{
  surf_action_change_state(action, SURF_ACTION_FAILED);
  return;
}

static void action_change_state(surf_action_t action,
                                e_surf_action_state_t state)
{
/*   if((state==SURF_ACTION_DONE) || (state==SURF_ACTION_FAILED)) */
/*     if(((surf_action_cpu_Cas01_t)action)->variable) { */
/*       lmm_variable_disable(cpu_maxmin_system, ((surf_action_cpu_Cas01_t)action)->variable); */
/*       ((surf_action_cpu_Cas01_t)action)->variable = NULL; */
/*     } */

  surf_action_change_state(action, state);
  return;
}

static double share_resources(double now)
{
  s_surf_action_cpu_Cas01_t action;
  return generic_maxmin_share_resources(surf_cpu_model->common_public.states.
                                        running_action_set,
                                        xbt_swag_offset(action, variable),
                                        cpu_maxmin_system, lmm_solve);
}

static void update_actions_state(double now, double delta)
{
  surf_action_cpu_Cas01_t action = NULL;
  surf_action_cpu_Cas01_t next_action = NULL;
  xbt_swag_t running_actions =
    surf_cpu_model->common_public.states.running_action_set;
  /* FIXME: UNUSED
     xbt_swag_t failed_actions =
     surf_cpu_model->common_public.states.failed_action_set;
   */

  xbt_swag_foreach_safe(action, next_action, running_actions) {
    double_update(&(action->generic_action.remains),
                  lmm_variable_getvalue(action->variable) * delta);
    if (action->generic_action.max_duration != NO_MAX_DURATION)
      double_update(&(action->generic_action.max_duration), delta);
    if ((action->generic_action.remains <= 0) &&
        (lmm_get_variable_weight(action->variable) > 0)) {
      action->generic_action.finish = surf_get_clock();
      action_change_state((surf_action_t) action, SURF_ACTION_DONE);
    } else if ((action->generic_action.max_duration != NO_MAX_DURATION) &&
               (action->generic_action.max_duration <= 0)) {
      action->generic_action.finish = surf_get_clock();
      action_change_state((surf_action_t) action, SURF_ACTION_DONE);
    }
  }

  return;
}

static void update_resource_state(void *id,
                                  tmgr_trace_event_t event_type,
                                  double value, double date)
{
  cpu_Cas01_t cpu = id;

  if (event_type == cpu->power_event) {
    cpu->power_current = value;
    lmm_update_constraint_bound(cpu_maxmin_system, cpu->constraint,
                                cpu->power_current * cpu->power_scale);
  } else if (event_type == cpu->state_event) {
    if (value > 0)
      cpu->state_current = SURF_CPU_ON;
    else {
      lmm_constraint_t cnst = cpu->constraint;
      lmm_variable_t var = NULL;
      lmm_element_t elem = NULL;

      cpu->state_current = SURF_CPU_OFF;

      while ((var = lmm_get_var_from_cnst(cpu_maxmin_system, cnst, &elem))) {
        surf_action_t action = lmm_variable_id(var);

        if (surf_action_get_state(action) == SURF_ACTION_RUNNING ||
            surf_action_get_state(action) == SURF_ACTION_READY ||
            surf_action_get_state(action) == SURF_ACTION_NOT_IN_THE_SYSTEM) {
          action->finish = date;
          action_change_state(action, SURF_ACTION_FAILED);
        }
      }
    }
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

  XBT_IN2("(%s,%g)", CPU->name, size);
  action = xbt_new0(s_surf_action_cpu_Cas01_t, 1);

  action->generic_action.refcount = 1;
  action->generic_action.cost = size;
  action->generic_action.remains = size;
  action->generic_action.priority = 1.0;
  action->generic_action.max_duration = NO_MAX_DURATION;
  action->generic_action.start = surf_get_clock();
  action->generic_action.finish = -1.0;
  action->generic_action.model_type = (surf_model_t) surf_cpu_model;
  action->suspended = 0;        /* Should be useless because of the
                                   calloc but it seems to help valgrind... */

  if (CPU->state_current == SURF_CPU_ON)
    action->generic_action.state_set =
      surf_cpu_model->common_public.states.running_action_set;
  else
    action->generic_action.state_set =
      surf_cpu_model->common_public.states.failed_action_set;

  xbt_swag_insert(action, action->generic_action.state_set);

  action->variable = lmm_variable_new(cpu_maxmin_system, action,
                                      action->generic_action.priority,
                                      -1.0, 1);
  lmm_expand(cpu_maxmin_system, CPU->constraint, action->variable, 1.0);
  XBT_OUT;
  return (surf_action_t) action;
}

static surf_action_t action_sleep(void *cpu, double duration)
{
  surf_action_cpu_Cas01_t action = NULL;

  if (duration > 0)
    duration = MAX(duration, MAXMIN_PRECISION);

  XBT_IN2("(%s,%g)", ((cpu_Cas01_t) cpu)->name, duration);
  action = (surf_action_cpu_Cas01_t) execute(cpu, 1.0);
  action->generic_action.max_duration = duration;
  action->suspended = 2;
  if (duration == NO_MAX_DURATION) {
    /* Move to the *end* of the corresponding action set. This convention
       is used to speed up update_resource_state  */
    xbt_swag_remove(action, ((surf_action_t) action)->state_set);
    ((surf_action_t) action)->state_set =
      running_action_set_that_does_not_need_being_checked;
    xbt_swag_insert(action, ((surf_action_t) action)->state_set);
  }

  lmm_update_variable_weight(cpu_maxmin_system, action->variable, 0.0);
  XBT_OUT;
  return (surf_action_t) action;
}

static void action_suspend(surf_action_t action)
{
  XBT_IN1("(%p)", action);
  if (((surf_action_cpu_Cas01_t) action)->suspended != 2) {
    lmm_update_variable_weight(cpu_maxmin_system,
                               ((surf_action_cpu_Cas01_t) action)->variable,
                               0.0);
    ((surf_action_cpu_Cas01_t) action)->suspended = 1;
  }
  XBT_OUT;
}

static void action_resume(surf_action_t action)
{
  XBT_IN1("(%p)", action);
  if (((surf_action_cpu_Cas01_t) action)->suspended != 2) {
    lmm_update_variable_weight(cpu_maxmin_system,
                               ((surf_action_cpu_Cas01_t) action)->variable,
                               action->priority);
    ((surf_action_cpu_Cas01_t) action)->suspended = 0;
  }
  XBT_OUT;
}

static int action_is_suspended(surf_action_t action)
{
  return (((surf_action_cpu_Cas01_t) action)->suspended == 1);
}

static void action_set_max_duration(surf_action_t action, double duration)
{
  XBT_IN2("(%p,%g)", action, duration);
  action->max_duration = duration;
  XBT_OUT;
}

static void action_set_priority(surf_action_t action, double priority)
{
  XBT_IN2("(%p,%g)", action, priority);
  action->priority = priority;
  lmm_update_variable_weight(cpu_maxmin_system,
                             ((surf_action_cpu_Cas01_t) action)->variable,
                             priority);

  XBT_OUT;
}

static e_surf_cpu_state_t get_state(void *cpu)
{
  return ((cpu_Cas01_t) cpu)->state_current;
}

static double get_speed(void *cpu, double load)
{
  return load * (((cpu_Cas01_t) cpu)->power_scale);
}

static double get_available_speed(void *cpu)
{
  /* number between 0 and 1 */
  return ((cpu_Cas01_t) cpu)->power_current;
}

static xbt_dict_t get_properties(void *cpu)
{
  return ((cpu_Cas01_t) cpu)->properties;
}

static void finalize(void)
{
  lmm_system_free(cpu_maxmin_system);
  cpu_maxmin_system = NULL;

  surf_model_exit((surf_model_t) surf_cpu_model);

  xbt_swag_free(running_action_set_that_does_not_need_being_checked);
  running_action_set_that_does_not_need_being_checked = NULL;
  free(surf_cpu_model->extension_public);

  free(surf_cpu_model);
  surf_cpu_model = NULL;
}

static void surf_cpu_model_init_internal(void)
{
  s_surf_action_t action;

  surf_cpu_model = xbt_new0(s_surf_cpu_model_t, 1);

  surf_model_init((surf_model_t) surf_cpu_model);

  surf_cpu_model->extension_public =
    xbt_new0(s_surf_cpu_model_extension_public_t, 1);

  running_action_set_that_does_not_need_being_checked =
    xbt_swag_new(xbt_swag_offset(action, state_hookup));

  surf_cpu_model->common_public.get_resource_name = get_resource_name;
  surf_cpu_model->common_public.action_get_state = surf_action_get_state;
  surf_cpu_model->common_public.action_get_start_time =
    surf_action_get_start_time;
  surf_cpu_model->common_public.action_get_finish_time =
    surf_action_get_finish_time;
  surf_cpu_model->common_public.action_free = action_free;
  surf_cpu_model->common_public.action_use = action_use;
  surf_cpu_model->common_public.action_cancel = action_cancel;
  surf_cpu_model->common_public.action_change_state = action_change_state;
  surf_cpu_model->common_public.action_set_data = surf_action_set_data;
  surf_cpu_model->common_public.name = "CPU";

  surf_cpu_model->common_private->resource_used = resource_used;
  surf_cpu_model->common_private->share_resources = share_resources;
  surf_cpu_model->common_private->update_actions_state = update_actions_state;
  surf_cpu_model->common_private->update_resource_state =
    update_resource_state;
  surf_cpu_model->common_private->finalize = finalize;

  surf_cpu_model->common_public.suspend = action_suspend;
  surf_cpu_model->common_public.resume = action_resume;
  surf_cpu_model->common_public.is_suspended = action_is_suspended;
  surf_cpu_model->common_public.set_max_duration = action_set_max_duration;
  surf_cpu_model->common_public.set_priority = action_set_priority;
  surf_cpu_model->extension_public->execute = execute;
  surf_cpu_model->extension_public->sleep = action_sleep;

  surf_cpu_model->extension_public->get_state = get_state;
  surf_cpu_model->extension_public->get_speed = get_speed;
  surf_cpu_model->extension_public->get_available_speed = get_available_speed;
  /*manage the properties of the cpu */
  surf_cpu_model->common_public.get_properties = get_properties;

  if (!cpu_maxmin_system)
    cpu_maxmin_system = lmm_system_new();
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
void surf_cpu_model_init_Cas01(const char *filename)
{
  if (surf_cpu_model)
    return;
  surf_cpu_model_init_internal();
  define_callbacks(filename);
  xbt_dynar_push(model_list, &surf_cpu_model);
}
