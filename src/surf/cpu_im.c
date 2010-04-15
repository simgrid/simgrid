/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_private.h"

#undef GENERIC_LMM_ACTION
#undef GENERIC_ACTION
#undef ACTION_GET_CPU
#define GENERIC_LMM_ACTION(action) action->generic_lmm_action
#define GENERIC_ACTION(action) GENERIC_LMM_ACTION(action).generic_action
#define ACTION_GET_CPU(action) ((surf_action_cpu_Cas01_im_t) action)->cpu

typedef struct surf_action_cpu_cas01_im {
  s_surf_action_lmm_t generic_lmm_action;
  s_xbt_swag_hookup_t cpu_list_hookup;
  int index_heap;
  void *cpu;
} s_surf_action_cpu_Cas01_im_t, *surf_action_cpu_Cas01_im_t;

typedef struct cpu_Cas01_im {
  s_surf_resource_t generic_resource;
  s_xbt_swag_hookup_t modified_cpu_hookup;
  double power_peak;
  double power_scale;
  tmgr_trace_event_t power_event;
  e_surf_resource_state_t state_current;
  tmgr_trace_event_t state_event;
  lmm_constraint_t constraint;
  xbt_swag_t action_set;
  double last_update;
} s_cpu_Cas01_im_t, *cpu_Cas01_im_t;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_cpu_im, surf,
                                "Logging specific to the SURF CPU IMPROVED module");


lmm_system_t cpu_im_maxmin_system = NULL;
static xbt_swag_t cpu_im_modified_cpu = NULL;
static xbt_heap_t cpu_im_action_heap = NULL;
extern int sg_maxmin_selective_update;


static xbt_swag_t cpu_im_running_action_set_that_does_not_need_being_checked = NULL;

static cpu_Cas01_im_t cpu_im_new(char *name, double power_peak,
                              double power_scale,
                              tmgr_trace_t power_trace,
                              e_surf_resource_state_t state_initial,
                              tmgr_trace_t state_trace,
                              xbt_dict_t cpu_properties)
{
#ifdef HAVE_TRACING
  TRACE_surf_host_declaration (name, power_scale * power_peak);
#endif

  cpu_Cas01_im_t cpu = xbt_new0(s_cpu_Cas01_im_t, 1);
  s_surf_action_cpu_Cas01_im_t action;
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
    lmm_constraint_new(cpu_im_maxmin_system, cpu,
                       cpu->power_scale * cpu->power_peak);

  xbt_dict_set(surf_model_resource_set(surf_cpu_model), name, cpu,
               surf_resource_free);
  cpu->action_set = xbt_swag_new(xbt_swag_offset(action, cpu_list_hookup));

  return cpu;
}


static void parse_cpu_im_init(void)
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
  cpu_im_new(xbt_strdup(A_surfxml_host_id), power_peak, power_scale,
          power_trace, state_initial, state_trace, current_property_set);

}

static void cpu_im_add_traces_cpu(void)
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
    cpu_Cas01_im_t host = surf_model_resource_by_name(surf_cpu_model, elm);

    xbt_assert1(host, "Host %s undefined", elm);
    xbt_assert1(trace, "Trace %s undefined", trace_name);

    host->state_event = tmgr_history_add_trace(history, trace, 0.0, 0, host);
  }

  xbt_dict_foreach(trace_connect_list_power, cursor, trace_name, elm) {
    tmgr_trace_t trace = xbt_dict_get_or_null(traces_set_list, trace_name);
    cpu_Cas01_im_t host = surf_model_resource_by_name(surf_cpu_model, elm);

    xbt_assert1(host, "Host %s undefined", elm);
    xbt_assert1(trace, "Trace %s undefined", trace_name);

    host->power_event = tmgr_history_add_trace(history, trace, 0.0, 0, host);
  }
}

static void cpu_im_define_callbacks(const char *file)
{
  surf_parse_reset_parser();
  surfxml_add_callback(STag_surfxml_host_cb_list, parse_cpu_im_init);
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &cpu_im_add_traces_cpu);
}

static int cpu_im_resource_used(void *resource_id)
{
  return lmm_constraint_used(cpu_im_maxmin_system,
                             ((cpu_Cas01_im_t) resource_id)->constraint);
}

static int cpu_im_action_unref(surf_action_t action)
{
  action->refcount--;
  if (!action->refcount) {
    xbt_swag_remove(action, action->state_set);
    if (((surf_action_lmm_t) action)->variable)
      lmm_variable_free(cpu_im_maxmin_system,
                        ((surf_action_lmm_t) action)->variable);
    /* remove from heap */
    xbt_heap_remove(cpu_im_action_heap,
                    ((surf_action_cpu_Cas01_im_t) action)->index_heap);
    xbt_swag_remove(action,
                    ((cpu_Cas01_im_t) ACTION_GET_CPU(action))->action_set);
    xbt_swag_insert(ACTION_GET_CPU(action), cpu_im_modified_cpu);
    free(action);
    return 1;
  }
  return 0;
}

static void cpu_im_action_cancel(surf_action_t action)
{
  surf_action_state_set(action, SURF_ACTION_FAILED);
  xbt_heap_remove(cpu_im_action_heap,
                  ((surf_action_cpu_Cas01_im_t) action)->index_heap);
  xbt_swag_remove(action,
                  ((cpu_Cas01_im_t) ACTION_GET_CPU(action))->action_set);
  return;
}

static void cpu_im_cpu_action_state_set(surf_action_t action,
                                 e_surf_action_state_t state)
{
/*   if((state==SURF_ACTION_DONE) || (state==SURF_ACTION_FAILED)) */
/*     if(((surf_action_lmm_t)action)->variable) { */
/*       lmm_variable_disable(cpu_im_maxmin_system, ((surf_action_lmm_t)action)->variable); */
/*       ((surf_action_lmm_t)action)->variable = NULL; */
/*     } */

  surf_action_state_set(action, state);
  return;
}

static void cpu_im_update_remains(cpu_Cas01_im_t cpu, double now)
{
  surf_action_cpu_Cas01_im_t action;

  if (cpu->last_update >= now)
    return;
  xbt_swag_foreach(action, cpu->action_set) {
    if (GENERIC_ACTION(action).state_set !=
        surf_cpu_model->states.running_action_set)
      continue;

    /* bogus priority, skip it */
    if (GENERIC_ACTION(action).priority <= 0)
      continue;

    if (GENERIC_ACTION(action).remains > 0) {
      double_update(&(GENERIC_ACTION(action).remains),
                    lmm_variable_getvalue(GENERIC_LMM_ACTION
                                          (action).variable) * (now -
                                                                cpu->last_update));
#ifdef HAVE_TRACING
      TRACE_surf_host_set_utilization (cpu->generic_resource.name,
                action->generic_lmm_action.generic_action.data, lmm_variable_getvalue(GENERIC_LMM_ACTION(action).variable), cpu->last_update, now-cpu->last_update);
#endif
      DEBUG2("Update action(%p) remains %lf", action,
             GENERIC_ACTION(action).remains);
    }
  }
  cpu->last_update = now;
}

static double cpu_im_share_resources(double now)
{
  surf_action_cpu_Cas01_im_t action;
  double min;
  double value;
  cpu_Cas01_im_t cpu, cpu_next;

  xbt_swag_foreach(cpu, cpu_im_modified_cpu)
    cpu_im_update_remains(cpu, now);

  lmm_solve(cpu_im_maxmin_system);

  xbt_swag_foreach_safe(cpu, cpu_next, cpu_im_modified_cpu) {
    xbt_swag_foreach(action, cpu->action_set) {
      if (GENERIC_ACTION(action).state_set !=
          surf_cpu_model->states.running_action_set)
        continue;

      /* bogus priority, skip it */
      if (GENERIC_ACTION(action).priority <= 0)
        continue;

      min = -1;
      value = lmm_variable_getvalue(GENERIC_LMM_ACTION(action).variable);
      if (value > 0) {
        if (GENERIC_ACTION(action).remains > 0)
          value = GENERIC_ACTION(action).remains / value;
        else
          value = 0.0;
      }
      if (value > 0)
        min = now + value;

      if ((GENERIC_ACTION(action).max_duration != NO_MAX_DURATION)
          && (min == -1
              || GENERIC_ACTION(action).start +
              GENERIC_ACTION(action).max_duration < min))
        min =
          GENERIC_ACTION(action).start + GENERIC_ACTION(action).max_duration;

      DEBUG4("Action(%p) Start %lf Finish %lf Max_duration %lf", action,
             GENERIC_ACTION(action).start, now + value,
             GENERIC_ACTION(action).max_duration);

      if (action->index_heap >= 0) {
        surf_action_cpu_Cas01_im_t heap_act =
          xbt_heap_remove(cpu_im_action_heap, action->index_heap);
        if (heap_act != action)
          DIE_IMPOSSIBLE;
      }
      if (min != -1) {
        xbt_heap_push(cpu_im_action_heap, action, min);
        DEBUG2("Insert at heap action(%p) min %lf", action, min);
      }
    }
    xbt_swag_remove(cpu, cpu_im_modified_cpu);
  }
  return xbt_heap_size(cpu_im_action_heap) >
    0 ? xbt_heap_maxkey(cpu_im_action_heap) - now : -1;
}

static void cpu_im_update_actions_state(double now, double delta)
{
  surf_action_cpu_Cas01_im_t action;

  while ((xbt_heap_size(cpu_im_action_heap) > 0)
         && (double_equals(xbt_heap_maxkey(cpu_im_action_heap), now))) {
    action = xbt_heap_pop(cpu_im_action_heap);
    DEBUG1("Action %p: finish", action);
    GENERIC_ACTION(action).finish = surf_get_clock();
    /* set the remains to 0 due to precision problems when updating the remaining amount */
#ifdef HAVE_TRACING
    TRACE_surf_host_set_utilization (((cpu_Cas01_im_t)(action->cpu))->generic_resource.name,
              action->generic_lmm_action.generic_action.data, lmm_variable_getvalue(GENERIC_LMM_ACTION(action).variable), ((cpu_Cas01_im_t)(action->cpu))->last_update, now-((cpu_Cas01_im_t)(action->cpu))->last_update);
#endif
    GENERIC_ACTION(action).remains = 0;
    cpu_im_cpu_action_state_set((surf_action_t) action, SURF_ACTION_DONE);
    cpu_im_update_remains(action->cpu, surf_get_clock());
  }
  return;
}

static void cpu_im_update_resource_state(void *id,
                                  tmgr_trace_event_t event_type,
                                  double value, double date)
{
  cpu_Cas01_im_t cpu = id;

  if (event_type == cpu->power_event) {
    cpu->power_scale = value;
    lmm_update_constraint_bound(cpu_im_maxmin_system, cpu->constraint,
                                cpu->power_scale * cpu->power_peak);
#ifdef HAVE_TRACING
    TRACE_surf_host_set_power (date, cpu->generic_resource.name, cpu->power_scale * cpu->power_peak);
#endif
    xbt_swag_insert(cpu, cpu_im_modified_cpu);
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

      while ((var = lmm_get_var_from_cnst(cpu_im_maxmin_system, cnst, &elem))) {
        surf_action_t action = lmm_variable_id(var);

        if (surf_action_state_get(action) == SURF_ACTION_RUNNING ||
            surf_action_state_get(action) == SURF_ACTION_READY ||
            surf_action_state_get(action) == SURF_ACTION_NOT_IN_THE_SYSTEM) {
          action->finish = date;
          cpu_im_cpu_action_state_set(action, SURF_ACTION_FAILED);
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

static surf_action_t cpu_im_execute(void *cpu, double size)
{
  surf_action_cpu_Cas01_im_t action = NULL;
  cpu_Cas01_im_t CPU = cpu;

  XBT_IN2("(%s,%g)", surf_resource_name(CPU), size);
  action =
    surf_action_new(sizeof(s_surf_action_cpu_Cas01_im_t), size,
                    surf_cpu_model, CPU->state_current != SURF_RESOURCE_ON);

  GENERIC_LMM_ACTION(action).suspended = 0;     /* Should be useless because of the
                                                   calloc but it seems to help valgrind... */

  GENERIC_LMM_ACTION(action).variable =
    lmm_variable_new(cpu_im_maxmin_system, action,
                     GENERIC_ACTION(action).priority, -1.0, 1);
  action->index_heap = -1;
  action->cpu = CPU;
  xbt_swag_insert(CPU, cpu_im_modified_cpu);
  xbt_swag_insert(action, CPU->action_set);
  lmm_expand(cpu_im_maxmin_system, CPU->constraint,
             GENERIC_LMM_ACTION(action).variable, 1.0);
  XBT_OUT;
  return (surf_action_t) action;
}

static surf_action_t cpu_im_action_sleep(void *cpu, double duration)
{
  surf_action_cpu_Cas01_im_t action = NULL;

  if (duration > 0)
    duration = MAX(duration, MAXMIN_PRECISION);

  XBT_IN2("(%s,%g)", surf_resource_name(cpu), duration);
  action = (surf_action_cpu_Cas01_im_t) cpu_im_execute(cpu, 1.0);
  GENERIC_ACTION(action).max_duration = duration;
  GENERIC_LMM_ACTION(action).suspended = 2;
  if (duration == NO_MAX_DURATION) {
    /* Move to the *end* of the corresponding action set. This convention
       is used to speed up update_resource_state  */
    xbt_swag_remove(action, ((surf_action_t) action)->state_set);
    ((surf_action_t) action)->state_set =
      cpu_im_running_action_set_that_does_not_need_being_checked;
    xbt_swag_insert(action, ((surf_action_t) action)->state_set);
  }

  lmm_update_variable_weight(cpu_im_maxmin_system,
                             GENERIC_LMM_ACTION(action).variable, 0.0);
  xbt_swag_insert(cpu, cpu_im_modified_cpu);
  XBT_OUT;
  return (surf_action_t) action;
}

static void cpu_im_action_suspend(surf_action_t action)
{
  XBT_IN1("(%p)", action);
  if (((surf_action_lmm_t) action)->suspended != 2) {
    lmm_update_variable_weight(cpu_im_maxmin_system,
                               ((surf_action_lmm_t) action)->variable, 0.0);
    ((surf_action_lmm_t) action)->suspended = 1;
    xbt_heap_remove(cpu_im_action_heap,
                    ((surf_action_cpu_Cas01_im_t) action)->index_heap);
    xbt_swag_insert(ACTION_GET_CPU(action), cpu_im_modified_cpu);
  }
  XBT_OUT;
}

static void cpu_im_action_resume(surf_action_t action)
{
  XBT_IN1("(%p)", action);
  if (((surf_action_lmm_t) action)->suspended != 2) {
    lmm_update_variable_weight(cpu_im_maxmin_system,
                               ((surf_action_lmm_t) action)->variable,
                               action->priority);
    ((surf_action_lmm_t) action)->suspended = 0;
    xbt_swag_insert(ACTION_GET_CPU(action), cpu_im_modified_cpu);
  }
  XBT_OUT;
}

static int cpu_im_action_is_suspended(surf_action_t action)
{
  return (((surf_action_lmm_t) action)->suspended == 1);
}

static void cpu_im_action_set_max_duration(surf_action_t action, double duration)
{
  XBT_IN2("(%p,%g)", action, duration);

  action->max_duration = duration;
  /* insert cpu in modified_cpu set to notice the max duration change */
  xbt_swag_insert(ACTION_GET_CPU(action), cpu_im_modified_cpu);
  XBT_OUT;
}

static void cpu_im_action_set_priority(surf_action_t action, double priority)
{
  XBT_IN2("(%p,%g)", action, priority);
  action->priority = priority;
  lmm_update_variable_weight(cpu_im_maxmin_system,
                             ((surf_action_lmm_t) action)->variable,
                             priority);

  xbt_swag_insert(ACTION_GET_CPU(action), cpu_im_modified_cpu);
  XBT_OUT;
}

static double cpu_im_action_get_remains(surf_action_t action)
{
  XBT_IN1("(%p)", action);
  /* update remains before return it */
  cpu_im_update_remains(ACTION_GET_CPU(action), surf_get_clock());
  return action->remains;
  XBT_OUT;
}

static e_surf_resource_state_t cpu_im_get_state(void *cpu)
{
  return ((cpu_Cas01_im_t) cpu)->state_current;
}

static double cpu_im_get_speed(void *cpu, double load)
{
  return load * (((cpu_Cas01_im_t) cpu)->power_peak);
}

static double cpu_im_get_available_speed(void *cpu)
{
  /* number between 0 and 1 */
  return ((cpu_Cas01_im_t) cpu)->power_scale;
}

static void cpu_im_action_update_index_heap(void *action, int i)
{
  ((surf_action_cpu_Cas01_im_t) action)->index_heap = i;
}

static void cpu_im_finalize(void)
{
  void *cpu;
  xbt_dict_cursor_t cursor;
  char *key;
  xbt_dict_foreach(surf_model_resource_set(surf_cpu_model), cursor, key, cpu) {
    cpu_Cas01_im_t CPU = cpu;
    xbt_swag_free(CPU->action_set);
  }

  lmm_system_free(cpu_im_maxmin_system);
  cpu_im_maxmin_system = NULL;

  surf_model_exit(surf_cpu_model);
  surf_cpu_model = NULL;

  xbt_swag_free(cpu_im_running_action_set_that_does_not_need_being_checked);
  cpu_im_running_action_set_that_does_not_need_being_checked = NULL;
  xbt_heap_free(cpu_im_action_heap);
  xbt_swag_free(cpu_im_modified_cpu);
}

static void surf_cpu_im_model_init_internal(void)
{
  s_surf_action_t action;
  s_cpu_Cas01_im_t cpu;

  surf_cpu_model = surf_model_init();

  cpu_im_running_action_set_that_does_not_need_being_checked =
    xbt_swag_new(xbt_swag_offset(action, state_hookup));

  surf_cpu_model->name = "CPU_IM";

  surf_cpu_model->action_unref = cpu_im_action_unref;
  surf_cpu_model->action_cancel = cpu_im_action_cancel;
  surf_cpu_model->action_state_set = cpu_im_cpu_action_state_set;

  surf_cpu_model->model_private->resource_used = cpu_im_resource_used;
  surf_cpu_model->model_private->share_resources = cpu_im_share_resources;
  surf_cpu_model->model_private->update_actions_state = cpu_im_update_actions_state;
  surf_cpu_model->model_private->update_resource_state =
    cpu_im_update_resource_state;
  surf_cpu_model->model_private->finalize = cpu_im_finalize;

  surf_cpu_model->suspend = cpu_im_action_suspend;
  surf_cpu_model->resume = cpu_im_action_resume;
  surf_cpu_model->is_suspended = cpu_im_action_is_suspended;
  surf_cpu_model->set_max_duration = cpu_im_action_set_max_duration;
  surf_cpu_model->set_priority = cpu_im_action_set_priority;
  surf_cpu_model->get_remains = cpu_im_action_get_remains;

  surf_cpu_model->extension.cpu.execute = cpu_im_execute;
  surf_cpu_model->extension.cpu.sleep = cpu_im_action_sleep;

  surf_cpu_model->extension.cpu.get_state = cpu_im_get_state;
  surf_cpu_model->extension.cpu.get_speed = cpu_im_get_speed;
  surf_cpu_model->extension.cpu.get_available_speed = cpu_im_get_available_speed;

  if (!cpu_im_maxmin_system) {
    sg_maxmin_selective_update = 1;
    cpu_im_maxmin_system = lmm_system_new();
  }
  cpu_im_action_heap = xbt_heap_new(8, NULL);
  xbt_heap_set_update_callback(cpu_im_action_heap, cpu_im_action_update_index_heap);
  cpu_im_modified_cpu = xbt_swag_new(xbt_swag_offset(cpu, modified_cpu_hookup));
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
void surf_cpu_model_init_Cas01_im(const char *filename)
{
  if (surf_cpu_model)
    return;
  surf_cpu_im_model_init_internal();
  cpu_im_define_callbacks(filename);
  xbt_dynar_push(model_list, &surf_cpu_model);
}
