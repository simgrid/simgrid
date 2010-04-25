/* Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_private.h"

typedef s_surf_action_lmm_t s_surf_action_cpu_Cas01_t,
  *surf_action_cpu_Cas01_t;

typedef struct cpu_Cas01 {
  s_surf_resource_t generic_resource;
  double power_peak;
  double power_scale;
  tmgr_trace_event_t power_event;
  e_surf_resource_state_t state_current;
  tmgr_trace_event_t state_event;
  lmm_constraint_t constraint;
} s_cpu_Cas01_t, *cpu_Cas01_t;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_cpu, surf,
                                "Logging specific to the SURF CPU module");



surf_model_t surf_cpu_model = NULL;
lmm_system_t cpu_maxmin_system = NULL;


static xbt_swag_t cpu_running_action_set_that_does_not_need_being_checked = NULL;

static cpu_Cas01_t cpu_new(char *name, double power_peak,
                           double power_scale,
                           tmgr_trace_t power_trace,
                           e_surf_resource_state_t state_initial,
                           tmgr_trace_t state_trace,
                           xbt_dict_t cpu_properties)
{
  cpu_Cas01_t cpu = xbt_new0(s_cpu_Cas01_t, 1);
  xbt_assert1(!surf_model_resource_by_name(surf_cpu_model, name),
              "Host '%s' declared several times in the platform file", name);
  cpu->generic_resource.model = surf_cpu_model;
  cpu->generic_resource.name = name;
  cpu->generic_resource.properties = cpu_properties;
  cpu->power_peak = power_peak;
  xbt_assert0(cpu->power_peak > 0, "Power has to be >0");
  cpu->power_scale = power_scale;
  if (power_trace)
    cpu->power_event =
      tmgr_history_add_trace(history, power_trace, 0.0, 0, cpu);

  cpu->state_current = state_initial;
  if (state_trace)
    cpu->state_event =
      tmgr_history_add_trace(history, state_trace, 0.0, 0, cpu);

  cpu->constraint =
    lmm_constraint_new(cpu_maxmin_system, cpu,
                       cpu->power_scale * cpu->power_peak);

  xbt_dict_set(surf_model_resource_set(surf_cpu_model), name, cpu,
               surf_resource_free);
#ifdef HAVE_TRACING
  TRACE_surf_host_declaration (name, cpu->power_scale * cpu->power_peak);
#endif

  return cpu;
}


static void parse_cpu_init(void)
{
  double power_peak = 0.0;
  double power_scale = 0.0;
  tmgr_trace_t power_trace = NULL;
  e_surf_resource_state_t state_initial = SURF_RESOURCE_OFF;
  tmgr_trace_t state_trace = NULL;

  power_peak = get_cpu_power(A_surfxml_host_power);
  surf_parse_get_double(&power_scale, A_surfxml_host_availability);
  power_trace = tmgr_trace_new(A_surfxml_host_availability_file);

  xbt_assert0((A_surfxml_host_state == A_surfxml_host_state_ON) ||
              (A_surfxml_host_state == A_surfxml_host_state_OFF),
              "Invalid state");
  if (A_surfxml_host_state == A_surfxml_host_state_ON)
    state_initial = SURF_RESOURCE_ON;
  if (A_surfxml_host_state == A_surfxml_host_state_OFF)
    state_initial = SURF_RESOURCE_OFF;
  state_trace = tmgr_trace_new(A_surfxml_host_state_file);

  current_property_set = xbt_dict_new();
  cpu_new(xbt_strdup(A_surfxml_host_id), power_peak, power_scale,
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
    cpu_Cas01_t host = surf_model_resource_by_name(surf_cpu_model, elm);

    xbt_assert1(host, "Host %s undefined", elm);
    xbt_assert1(trace, "Trace %s undefined", trace_name);

    host->state_event = tmgr_history_add_trace(history, trace, 0.0, 0, host);
  }

  xbt_dict_foreach(trace_connect_list_power, cursor, trace_name, elm) {
    tmgr_trace_t trace = xbt_dict_get_or_null(traces_set_list, trace_name);
    cpu_Cas01_t host = surf_model_resource_by_name(surf_cpu_model, elm);

    xbt_assert1(host, "Host %s undefined", elm);
    xbt_assert1(trace, "Trace %s undefined", trace_name);

    host->power_event = tmgr_history_add_trace(history, trace, 0.0, 0, host);
  }
}

static void cpu_define_callbacks(const char *file)
{
  surf_parse_reset_parser();
  surfxml_add_callback(STag_surfxml_host_cb_list, parse_cpu_init);
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &add_traces_cpu);
}

static int cpu_resource_used(void *resource_id)
{
  return lmm_constraint_used(cpu_maxmin_system,
                             ((cpu_Cas01_t) resource_id)->constraint);
}

static int cpu_action_unref(surf_action_t action)
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

static void cpu_action_cancel(surf_action_t action)
{
  surf_action_state_set(action, SURF_ACTION_FAILED);
  return;
}

static void cpu_action_state_set(surf_action_t action,
                                 e_surf_action_state_t state)
{
/*   if((state==SURF_ACTION_DONE) || (state==SURF_ACTION_FAILED)) */
/*     if(((surf_action_cpu_Cas01_t)action)->variable) { */
/*       lmm_variable_disable(cpu_maxmin_system, ((surf_action_cpu_Cas01_t)action)->variable); */
/*       ((surf_action_cpu_Cas01_t)action)->variable = NULL; */
/*     } */

  surf_action_state_set(action, state);
  return;
}

static double cpu_share_resources(double now)
{
  s_surf_action_cpu_Cas01_t action;
  return generic_maxmin_share_resources(surf_cpu_model->
                                        states.running_action_set,
                                        xbt_swag_offset(action, variable),
                                        cpu_maxmin_system, lmm_solve);
}

static void cpu_update_actions_state(double now, double delta)
{
  surf_action_cpu_Cas01_t action = NULL;
  surf_action_cpu_Cas01_t next_action = NULL;
  xbt_swag_t running_actions = surf_cpu_model->states.running_action_set;

  xbt_swag_foreach_safe(action, next_action, running_actions) {
#ifdef HAVE_TRACING
    cpu_Cas01_t x = lmm_constraint_id(lmm_get_cnst_from_var (cpu_maxmin_system, action->variable, 0));

    TRACE_surf_host_set_utilization (x->generic_resource.name,
              action->generic_action.data, lmm_variable_getvalue(action->variable), now-delta, delta);
#endif
    double_update(&(action->generic_action.remains),
                  lmm_variable_getvalue(action->variable) * delta);
    if (action->generic_action.max_duration != NO_MAX_DURATION)
      double_update(&(action->generic_action.max_duration), delta);
    if ((action->generic_action.remains <= 0) &&
        (lmm_get_variable_weight(action->variable) > 0)) {
      action->generic_action.finish = surf_get_clock();
      cpu_action_state_set((surf_action_t) action, SURF_ACTION_DONE);
    } else if ((action->generic_action.max_duration != NO_MAX_DURATION) &&
               (action->generic_action.max_duration <= 0)) {
      action->generic_action.finish = surf_get_clock();
      cpu_action_state_set((surf_action_t) action, SURF_ACTION_DONE);
    }
  }

  return;
}

static void cpu_update_resource_state(void *id,
                                  tmgr_trace_event_t event_type,
                                  double value, double date)
{
  cpu_Cas01_t cpu = id;

  if (event_type == cpu->power_event) {
    cpu->power_scale = value;
    lmm_update_constraint_bound(cpu_maxmin_system, cpu->constraint,
                                cpu->power_scale * cpu->power_peak);
#ifdef HAVE_TRACING
    TRACE_surf_host_set_power (date, cpu->generic_resource.name, cpu->power_scale * cpu->power_peak);
#endif
    if (tmgr_trace_event_free(event_type))
      cpu->power_event = NULL;
  } else if (event_type == cpu->state_event) {
    if (value > 0)
      cpu->state_current = SURF_RESOURCE_ON;
    else {
      lmm_constraint_t cnst = cpu->constraint;
      lmm_variable_t var = NULL;
      lmm_element_t elem = NULL;

      cpu->state_current = SURF_RESOURCE_OFF;

      while ((var = lmm_get_var_from_cnst(cpu_maxmin_system, cnst, &elem))) {
        surf_action_t action = lmm_variable_id(var);

        if (surf_action_state_get(action) == SURF_ACTION_RUNNING ||
            surf_action_state_get(action) == SURF_ACTION_READY ||
            surf_action_state_get(action) == SURF_ACTION_NOT_IN_THE_SYSTEM) {
          action->finish = date;
          cpu_action_state_set(action, SURF_ACTION_FAILED);
        }
      }
    }
    if (tmgr_trace_event_free(event_type))
      cpu->state_event = NULL;
  } else {
    CRITICAL0("Unknown event ! \n");
    xbt_abort();
  }

  return;
}

static surf_action_t cpu_execute(void *cpu, double size)
{
  surf_action_cpu_Cas01_t action = NULL;
  cpu_Cas01_t CPU = cpu;

  XBT_IN2("(%s,%g)", surf_resource_name(CPU), size);
  action =
    surf_action_new(sizeof(s_surf_action_cpu_Cas01_t), size, surf_cpu_model,
                    CPU->state_current != SURF_RESOURCE_ON);

  action->suspended = 0;        /* Should be useless because of the
                                   calloc but it seems to help valgrind... */

  action->variable = lmm_variable_new(cpu_maxmin_system, action,
                                      action->generic_action.priority,
                                      -1.0, 1);
  lmm_expand(cpu_maxmin_system, CPU->constraint, action->variable, 1.0);
  XBT_OUT;
  return (surf_action_t) action;
}

static surf_action_t cpu_action_sleep(void *cpu, double duration)
{
  surf_action_cpu_Cas01_t action = NULL;

  if (duration > 0)
    duration = MAX(duration, MAXMIN_PRECISION);

  XBT_IN2("(%s,%g)", surf_resource_name(cpu), duration);
  action = (surf_action_cpu_Cas01_t) cpu_execute(cpu, 1.0);
  action->generic_action.max_duration = duration;
  action->suspended = 2;
  if (duration == NO_MAX_DURATION) {
    /* Move to the *end* of the corresponding action set. This convention
       is used to speed up update_resource_state  */
    xbt_swag_remove(action, ((surf_action_t) action)->state_set);
    ((surf_action_t) action)->state_set =
      cpu_running_action_set_that_does_not_need_being_checked;
    xbt_swag_insert(action, ((surf_action_t) action)->state_set);
  }

  lmm_update_variable_weight(cpu_maxmin_system, action->variable, 0.0);
  XBT_OUT;
  return (surf_action_t) action;
}

static void cpu_action_suspend(surf_action_t action)
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

static void cpu_action_resume(surf_action_t action)
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

static int cpu_action_is_suspended(surf_action_t action)
{
  return (((surf_action_cpu_Cas01_t) action)->suspended == 1);
}

static void cpu_action_set_max_duration(surf_action_t action, double duration)
{
  XBT_IN2("(%p,%g)", action, duration);
  action->max_duration = duration;
  XBT_OUT;
}

static void cpu_action_set_priority(surf_action_t action, double priority)
{
  XBT_IN2("(%p,%g)", action, priority);
  action->priority = priority;
  lmm_update_variable_weight(cpu_maxmin_system,
                             ((surf_action_cpu_Cas01_t) action)->variable,
                             priority);

  XBT_OUT;
}

static double cpu_action_get_remains(surf_action_t action)
{
  XBT_IN1("(%p)", action);
  return action->remains;
  XBT_OUT;
}

static e_surf_resource_state_t cpu_get_state(void *cpu)
{
  return ((cpu_Cas01_t) cpu)->state_current;
}

static double cpu_get_speed(void *cpu, double load)
{
  return load * (((cpu_Cas01_t) cpu)->power_peak);
}

static double cpu_get_available_speed(void *cpu)
{
  /* number between 0 and 1 */
  return ((cpu_Cas01_t) cpu)->power_scale;
}

static void cpu_finalize(void)
{
  lmm_system_free(cpu_maxmin_system);
  cpu_maxmin_system = NULL;

  surf_model_exit(surf_cpu_model);
  surf_cpu_model = NULL;

  xbt_swag_free(cpu_running_action_set_that_does_not_need_being_checked);
  cpu_running_action_set_that_does_not_need_being_checked = NULL;
}

static void surf_cpu_model_init_internal(void)
{
  s_surf_action_t action;

  surf_cpu_model = surf_model_init();

  cpu_running_action_set_that_does_not_need_being_checked =
    xbt_swag_new(xbt_swag_offset(action, state_hookup));

  surf_cpu_model->name = "CPU";

  surf_cpu_model->action_unref = cpu_action_unref;
  surf_cpu_model->action_cancel = cpu_action_cancel;
  surf_cpu_model->action_state_set = cpu_action_state_set;

  surf_cpu_model->model_private->resource_used = cpu_resource_used;
  surf_cpu_model->model_private->share_resources = cpu_share_resources;
  surf_cpu_model->model_private->update_actions_state = cpu_update_actions_state;
  surf_cpu_model->model_private->update_resource_state =
    cpu_update_resource_state;
  surf_cpu_model->model_private->finalize = cpu_finalize;

  surf_cpu_model->suspend = cpu_action_suspend;
  surf_cpu_model->resume = cpu_action_resume;
  surf_cpu_model->is_suspended = cpu_action_is_suspended;
  surf_cpu_model->set_max_duration = cpu_action_set_max_duration;
  surf_cpu_model->set_priority = cpu_action_set_priority;
  surf_cpu_model->get_remains = cpu_action_get_remains;

  surf_cpu_model->extension.cpu.execute = cpu_execute;
  surf_cpu_model->extension.cpu.sleep = cpu_action_sleep;

  surf_cpu_model->extension.cpu.get_state = cpu_get_state;
  surf_cpu_model->extension.cpu.get_speed = cpu_get_speed;
  surf_cpu_model->extension.cpu.get_available_speed = cpu_get_available_speed;

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
  cpu_define_callbacks(filename);
  xbt_dynar_push(model_list, &surf_cpu_model);
}
