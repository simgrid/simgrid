/* Copyright (c) 2009-2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_private.h"
#include "surf/surf_resource.h"

surf_model_t surf_cpu_model = NULL;
lmm_system_t cpu_maxmin_system = NULL;
e_UM_t cpu_update_mechanism = UM_UNDEFINED;
static int cpu_selective_update = 0;

static xbt_swag_t cpu_modified_cpu = NULL;
static xbt_heap_t cpu_action_heap = NULL;

#undef GENERIC_LMM_ACTION
#undef GENERIC_ACTION
#undef ACTION_GET_CPU
#define GENERIC_LMM_ACTION(action) action->generic_lmm_action
#define GENERIC_ACTION(action) GENERIC_LMM_ACTION(action).generic_action
#define ACTION_GET_CPU(action) ((surf_action_cpu_Cas01_t) action)->cpu

typedef struct surf_action_cpu_cas01 {
  s_surf_action_lmm_t generic_lmm_action;
  s_xbt_swag_hookup_t cpu_list_hookup;
  int index_heap;
  void *cpu;
} s_surf_action_cpu_Cas01_t, *surf_action_cpu_Cas01_t;

typedef struct cpu_Cas01 {
  s_surf_resource_t generic_resource;
  s_xbt_swag_hookup_t modified_cpu_hookup;
  double power_peak;
  double power_scale;
  tmgr_trace_event_t power_event;
  int core;
  e_surf_resource_state_t state_current;
  tmgr_trace_event_t state_event;
  lmm_constraint_t constraint;
  xbt_swag_t action_set;
  double last_update;
} s_cpu_Cas01_t, *cpu_Cas01_t;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_cpu, surf,
                                "Logging specific to the SURF CPU IMPROVED module");



static xbt_swag_t
    cpu_running_action_set_that_does_not_need_being_checked = NULL;

static void *cpu_create_resource(const char *name, double power_peak,
                                 double power_scale,
                                 tmgr_trace_t power_trace,
                                 int core,
                                 e_surf_resource_state_t state_initial,
                                 tmgr_trace_t state_trace,
                                 xbt_dict_t cpu_properties)
{
  cpu_Cas01_t cpu = NULL;
  s_surf_action_cpu_Cas01_t action;

  xbt_assert(!surf_cpu_resource_by_name(name),
             "Host '%s' declared several times in the platform file",
             name);
  cpu = (cpu_Cas01_t) surf_resource_new(sizeof(s_cpu_Cas01_t),
                                        surf_cpu_model, name,
                                        cpu_properties);
  cpu->power_peak = power_peak;
  xbt_assert(cpu->power_peak > 0, "Power has to be >0");
  cpu->power_scale = power_scale;
  cpu->core = core;
  xbt_assert(core > 0, "Invalid number of cores %d", core);

  if (power_trace)
    cpu->power_event =
        tmgr_history_add_trace(history, power_trace, 0.0, 0, cpu);

  cpu->state_current = state_initial;
  if (state_trace)
    cpu->state_event =
        tmgr_history_add_trace(history, state_trace, 0.0, 0, cpu);

  cpu->constraint =
      lmm_constraint_new(cpu_maxmin_system, cpu,
                         cpu->core * cpu->power_scale * cpu->power_peak);

  xbt_lib_set(host_lib, name, SURF_CPU_LEVEL, cpu);
  if (cpu_update_mechanism == UM_LAZY)
    cpu->action_set =
        xbt_swag_new(xbt_swag_offset(action, cpu_list_hookup));

  return cpu;
}


static void parse_cpu_init(sg_platf_host_cbarg_t host)
{
  cpu_create_resource(host->id,
                      host->power_peak,
                      host->power_scale,
                      host->power_trace,
                      host->core_amount,
                      host->initial_state,
                      host->state_trace, host->properties);
}

static void cpu_add_traces_cpu(void)
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
    cpu_Cas01_t host = surf_cpu_resource_by_name(elm);

    xbt_assert(host, "Host %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    host->state_event =
        tmgr_history_add_trace(history, trace, 0.0, 0, host);
  }

  xbt_dict_foreach(trace_connect_list_power, cursor, trace_name, elm) {
    tmgr_trace_t trace = xbt_dict_get_or_null(traces_set_list, trace_name);
    cpu_Cas01_t host = surf_cpu_resource_by_name(elm);

    xbt_assert(host, "Host %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    host->power_event =
        tmgr_history_add_trace(history, trace, 0.0, 0, host);
  }
}

static void cpu_define_callbacks()
{
  sg_platf_host_add_cb(parse_cpu_init);
  sg_platf_postparse_add_cb(cpu_add_traces_cpu);
}

static int cpu_resource_used(void *resource)
{
  return lmm_constraint_used(cpu_maxmin_system,
                             ((cpu_Cas01_t) resource)->constraint);
}

static int cpu_action_unref(surf_action_t action)
{
  action->refcount--;
  if (!action->refcount) {
    xbt_swag_remove(action, action->state_set);
    if (((surf_action_lmm_t) action)->variable)
      lmm_variable_free(cpu_maxmin_system,
                        ((surf_action_lmm_t) action)->variable);
    if (cpu_update_mechanism == UM_LAZY) {
      /* remove from heap */
      xbt_heap_remove(cpu_action_heap,
                      ((surf_action_cpu_Cas01_t) action)->index_heap);
      xbt_swag_remove(action,
                      ((cpu_Cas01_t) ACTION_GET_CPU(action))->action_set);
      xbt_swag_insert(ACTION_GET_CPU(action), cpu_modified_cpu);
    }
#ifdef HAVE_TRACING
    xbt_free(action->category);
#endif
    surf_action_free(&action);
    return 1;
  }
  return 0;
}

static void cpu_action_cancel(surf_action_t action)
{
  surf_action_state_set(action, SURF_ACTION_FAILED);
  if (cpu_update_mechanism == UM_LAZY) {
    xbt_heap_remove(cpu_action_heap,
                    ((surf_action_cpu_Cas01_t) action)->index_heap);
    xbt_swag_remove(action,
                    ((cpu_Cas01_t) ACTION_GET_CPU(action))->action_set);
  }
  return;
}

static void cpu_cpu_action_state_set(surf_action_t action,
                                     e_surf_action_state_t state)
{
/*   if((state==SURF_ACTION_DONE) || (state==SURF_ACTION_FAILED)) */
/*     if(((surf_action_lmm_t)action)->variable) { */
/*       lmm_variable_disable(cpu_maxmin_system, ((surf_action_lmm_t)action)->variable); */
/*       ((surf_action_lmm_t)action)->variable = NULL; */
/*     } */

  surf_action_state_set(action, state);
  return;
}

static void cpu_update_remains(cpu_Cas01_t cpu, double now)
{
  surf_action_cpu_Cas01_t action;

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
                                                                cpu->
                                                                last_update));
#ifdef HAVE_TRACING
      if (TRACE_is_enabled()) {
        TRACE_surf_host_set_utilization(cpu->generic_resource.name,
                                        action->generic_lmm_action.
                                        generic_action.data,
                                        (surf_action_t) action,
                                        lmm_variable_getvalue
                                        (GENERIC_LMM_ACTION(action).
                                         variable), cpu->last_update,
                                        now - cpu->last_update);
      }
#endif
      XBT_DEBUG("Update action(%p) remains %lf", action,
                GENERIC_ACTION(action).remains);
    }
  }
  cpu->last_update = now;
}

static double cpu_share_resources_lazy(double now)
{
  surf_action_cpu_Cas01_t action;
  double min;
  double value;
  cpu_Cas01_t cpu, cpu_next;

  xbt_swag_foreach(cpu, cpu_modified_cpu)
      cpu_update_remains(cpu, now);

  lmm_solve(cpu_maxmin_system);

  xbt_swag_foreach_safe(cpu, cpu_next, cpu_modified_cpu) {
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
        if (GENERIC_ACTION(action).remains > 0) {
          value = GENERIC_ACTION(action).remains / value;
          min = now + value;
        } else {
          value = 0.0;
          min = now;
        }
      }

      if ((GENERIC_ACTION(action).max_duration != NO_MAX_DURATION)
          && (min == -1
              || GENERIC_ACTION(action).start +
              GENERIC_ACTION(action).max_duration < min))
        min =
            GENERIC_ACTION(action).start +
            GENERIC_ACTION(action).max_duration;

      XBT_DEBUG("Action(%p) Start %lf Finish %lf Max_duration %lf", action,
                GENERIC_ACTION(action).start, now + value,
                GENERIC_ACTION(action).max_duration);

      if (action->index_heap >= 0) {
        surf_action_cpu_Cas01_t heap_act =
            xbt_heap_remove(cpu_action_heap, action->index_heap);
        if (heap_act != action)
          DIE_IMPOSSIBLE;
      }
      if (min != -1) {
        xbt_heap_push(cpu_action_heap, action, min);
        XBT_DEBUG("Insert at heap action(%p) min %lf", action, min);
      }
    }
    xbt_swag_remove(cpu, cpu_modified_cpu);
  }
  return xbt_heap_size(cpu_action_heap) >
      0 ? xbt_heap_maxkey(cpu_action_heap) - now : -1;
}

static double cpu_share_resources_full(double now)
{
  s_surf_action_cpu_Cas01_t action;
  return generic_maxmin_share_resources(surf_cpu_model->states.
                                        running_action_set,
                                        xbt_swag_offset(action,
                                                        generic_lmm_action.
                                                        variable),
                                        cpu_maxmin_system, lmm_solve);
}

static void cpu_update_actions_state_lazy(double now, double delta)
{
  surf_action_cpu_Cas01_t action;
  while ((xbt_heap_size(cpu_action_heap) > 0)
         && (double_equals(xbt_heap_maxkey(cpu_action_heap), now))) {
    action = xbt_heap_pop(cpu_action_heap);
    XBT_DEBUG("Action %p: finish", action);
    GENERIC_ACTION(action).finish = surf_get_clock();
    /* set the remains to 0 due to precision problems when updating the remaining amount */
#ifdef HAVE_TRACING
    if (TRACE_is_enabled()) {
      cpu_Cas01_t cpu = ((cpu_Cas01_t) (action->cpu));
      TRACE_surf_host_set_utilization(cpu->generic_resource.name,
                                      GENERIC_LMM_ACTION(action).
                                      generic_action.data,
                                      (surf_action_t) action,
                                      lmm_variable_getvalue
                                      (GENERIC_LMM_ACTION(action).
                                       variable), cpu->last_update,
                                      now - cpu->last_update);
    }
#endif
    GENERIC_ACTION(action).remains = 0;
    cpu_cpu_action_state_set((surf_action_t) action, SURF_ACTION_DONE);
    cpu_update_remains(action->cpu, surf_get_clock());
  }
#ifdef HAVE_TRACING
  if (TRACE_is_enabled()) {
    //defining the last timestamp that we can safely dump to trace file
    //without losing the event ascending order (considering all CPU's)
    void **data;
    cpu_Cas01_t cpu;
    xbt_lib_cursor_t cursor;
    char *key;
    double smaller = -1;
    xbt_lib_foreach(host_lib, cursor, key, data) {
      if (data[SURF_CPU_LEVEL]) {
        cpu = data[SURF_CPU_LEVEL];
        if (smaller < 0) {
          smaller = cpu->last_update;
          continue;
        }
        if (cpu->last_update < smaller) {
          smaller = cpu->last_update;
        }
      }
    }
    if (smaller > 0) {
      TRACE_last_timestamp_to_dump = smaller;
    }
  }
#endif
  return;
}

static void cpu_update_actions_state_full(double now, double delta)
{
  surf_action_cpu_Cas01_t action = NULL;
  surf_action_cpu_Cas01_t next_action = NULL;
  xbt_swag_t running_actions = surf_cpu_model->states.running_action_set;
  xbt_swag_foreach_safe(action, next_action, running_actions) {
#ifdef HAVE_TRACING
    if (TRACE_is_enabled()) {
      cpu_Cas01_t x =
          lmm_constraint_id(lmm_get_cnst_from_var
                            (cpu_maxmin_system,
                             GENERIC_LMM_ACTION(action).variable, 0));

      TRACE_surf_host_set_utilization(x->generic_resource.name,
                                      GENERIC_ACTION(action).data,
                                      (surf_action_t) action,
                                      lmm_variable_getvalue
                                      (GENERIC_LMM_ACTION(action).
                                       variable), now - delta, delta);
      TRACE_last_timestamp_to_dump = now - delta;
    }
#endif
    double_update(&(GENERIC_ACTION(action).remains),
                  lmm_variable_getvalue(GENERIC_LMM_ACTION(action).
                                        variable) * delta);
    if (GENERIC_LMM_ACTION(action).generic_action.max_duration !=
        NO_MAX_DURATION)
      double_update(&(GENERIC_ACTION(action).max_duration), delta);
    if ((GENERIC_ACTION(action).remains <= 0) &&
        (lmm_get_variable_weight(GENERIC_LMM_ACTION(action).variable) >
         0)) {
      GENERIC_ACTION(action).finish = surf_get_clock();
      cpu_cpu_action_state_set((surf_action_t) action, SURF_ACTION_DONE);
    } else if ((GENERIC_ACTION(action).max_duration != NO_MAX_DURATION) &&
               (GENERIC_ACTION(action).max_duration <= 0)) {
      GENERIC_ACTION(action).finish = surf_get_clock();
      cpu_cpu_action_state_set((surf_action_t) action, SURF_ACTION_DONE);
    }
  }

  return;
}

static void cpu_update_resource_state(void *id,
                                      tmgr_trace_event_t event_type,
                                      double value, double date)
{
  cpu_Cas01_t cpu = id;
  lmm_variable_t var = NULL;
  lmm_element_t elem = NULL;

  if (event_type == cpu->power_event) {
    cpu->power_scale = value;
    lmm_update_constraint_bound(cpu_maxmin_system, cpu->constraint,
                                cpu->core * cpu->power_scale *
                                cpu->power_peak);
#ifdef HAVE_TRACING
    TRACE_surf_host_set_power(date, cpu->generic_resource.name,
                              cpu->core * cpu->power_scale *
                              cpu->power_peak);
#endif
    while ((var = lmm_get_var_from_cnst
            (cpu_maxmin_system, cpu->constraint, &elem))) {
      surf_action_cpu_Cas01_t action = lmm_variable_id(var);
      lmm_update_variable_bound(cpu_maxmin_system,
                                GENERIC_LMM_ACTION(action).variable,
                                cpu->power_scale * cpu->power_peak);
    }
    if (cpu_update_mechanism == UM_LAZY)
      xbt_swag_insert(cpu, cpu_modified_cpu);
    if (tmgr_trace_event_free(event_type))
      cpu->power_event = NULL;
  } else if (event_type == cpu->state_event) {
    if (value > 0)
      cpu->state_current = SURF_RESOURCE_ON;
    else {
      lmm_constraint_t cnst = cpu->constraint;

      cpu->state_current = SURF_RESOURCE_OFF;

      while ((var = lmm_get_var_from_cnst(cpu_maxmin_system, cnst, &elem))) {
        surf_action_t action = lmm_variable_id(var);

        if (surf_action_state_get(action) == SURF_ACTION_RUNNING ||
            surf_action_state_get(action) == SURF_ACTION_READY ||
            surf_action_state_get(action) ==
            SURF_ACTION_NOT_IN_THE_SYSTEM) {
          action->finish = date;
          cpu_cpu_action_state_set(action, SURF_ACTION_FAILED);
        }
      }
    }
    if (tmgr_trace_event_free(event_type))
      cpu->state_event = NULL;
  } else {
    XBT_CRITICAL("Unknown event ! \n");
    xbt_abort();
  }

  return;
}

static surf_action_t cpu_execute(void *cpu, double size)
{
  surf_action_cpu_Cas01_t action = NULL;
  cpu_Cas01_t CPU = cpu;

  XBT_IN("(%s,%g)", surf_resource_name(CPU), size);
  action =
      surf_action_new(sizeof(s_surf_action_cpu_Cas01_t), size,
                      surf_cpu_model,
                      CPU->state_current != SURF_RESOURCE_ON);

  GENERIC_LMM_ACTION(action).suspended = 0;     /* Should be useless because of the
                                                   calloc but it seems to help valgrind... */

  GENERIC_LMM_ACTION(action).variable =
      lmm_variable_new(cpu_maxmin_system, action,
                       GENERIC_ACTION(action).priority,
                       CPU->power_scale * CPU->power_peak, 1);
  if (cpu_update_mechanism == UM_LAZY) {
    action->index_heap = -1;
    action->cpu = CPU;
    xbt_swag_insert(CPU, cpu_modified_cpu);
    xbt_swag_insert(action, CPU->action_set);
  }
  lmm_expand(cpu_maxmin_system, CPU->constraint,
             GENERIC_LMM_ACTION(action).variable, 1.0);
  XBT_OUT();
  return (surf_action_t) action;
}

static surf_action_t cpu_action_sleep(void *cpu, double duration)
{
  surf_action_cpu_Cas01_t action = NULL;

  if (duration > 0)
    duration = MAX(duration, MAXMIN_PRECISION);

  XBT_IN("(%s,%g)", surf_resource_name(cpu), duration);
  action = (surf_action_cpu_Cas01_t) cpu_execute(cpu, 1.0);
  GENERIC_ACTION(action).max_duration = duration;
  GENERIC_LMM_ACTION(action).suspended = 2;
  if (duration == NO_MAX_DURATION) {
    /* Move to the *end* of the corresponding action set. This convention
       is used to speed up update_resource_state  */
    xbt_swag_remove(action, ((surf_action_t) action)->state_set);
    ((surf_action_t) action)->state_set =
        cpu_running_action_set_that_does_not_need_being_checked;
    xbt_swag_insert(action, ((surf_action_t) action)->state_set);
  }

  lmm_update_variable_weight(cpu_maxmin_system,
                             GENERIC_LMM_ACTION(action).variable, 0.0);
  if (cpu_update_mechanism == UM_LAZY)
    xbt_swag_insert(cpu, cpu_modified_cpu);
  XBT_OUT();
  return (surf_action_t) action;
}

static void cpu_action_suspend(surf_action_t action)
{
  XBT_IN("(%p)", action);
  if (((surf_action_lmm_t) action)->suspended != 2) {
    lmm_update_variable_weight(cpu_maxmin_system,
                               ((surf_action_lmm_t) action)->variable,
                               0.0);
    ((surf_action_lmm_t) action)->suspended = 1;
    if (cpu_update_mechanism == UM_LAZY) {
      xbt_heap_remove(cpu_action_heap,
                      ((surf_action_cpu_Cas01_t) action)->index_heap);
      xbt_swag_insert(ACTION_GET_CPU(action), cpu_modified_cpu);
    }
  }
  XBT_OUT();
}

static void cpu_action_resume(surf_action_t action)
{

  XBT_IN("(%p)", action);
  if (((surf_action_lmm_t) action)->suspended != 2) {
    lmm_update_variable_weight(cpu_maxmin_system,
                               ((surf_action_lmm_t) action)->variable,
                               action->priority);
    ((surf_action_lmm_t) action)->suspended = 0;
    if (cpu_update_mechanism == UM_LAZY)
      xbt_swag_insert(ACTION_GET_CPU(action), cpu_modified_cpu);
  }
  XBT_OUT();
}

static int cpu_action_is_suspended(surf_action_t action)
{
  return (((surf_action_lmm_t) action)->suspended == 1);
}

static void cpu_action_set_max_duration(surf_action_t action,
                                        double duration)
{
  XBT_IN("(%p,%g)", action, duration);

  action->max_duration = duration;
  /* insert cpu in modified_cpu set to notice the max duration change */
  if (cpu_update_mechanism == UM_LAZY)
    xbt_swag_insert(ACTION_GET_CPU(action), cpu_modified_cpu);
  XBT_OUT();
}

static void cpu_action_set_priority(surf_action_t action, double priority)
{
  XBT_IN("(%p,%g)", action, priority);
  action->priority = priority;
  lmm_update_variable_weight(cpu_maxmin_system,
                             ((surf_action_lmm_t) action)->variable,
                             priority);

  if (cpu_update_mechanism == UM_LAZY)
    xbt_swag_insert(ACTION_GET_CPU(action), cpu_modified_cpu);
  XBT_OUT();
}

#ifdef HAVE_TRACING
static void cpu_action_set_category(surf_action_t action,
                                    const char *category)
{
  XBT_IN("(%p,%s)", action, category);
  action->category = xbt_strdup(category);
  XBT_OUT();
}
#endif

static double cpu_action_get_remains(surf_action_t action)
{
  XBT_IN("(%p)", action);
  /* update remains before return it */
  if (cpu_update_mechanism == UM_LAZY)
    cpu_update_remains(ACTION_GET_CPU(action), surf_get_clock());
  XBT_OUT();
  return action->remains;
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

static void cpu_action_update_index_heap(void *action, int i)
{
  ((surf_action_cpu_Cas01_t) action)->index_heap = i;
}

static void cpu_finalize(void)
{
  void **cpu;
  xbt_lib_cursor_t cursor;
  char *key;

  xbt_lib_foreach(host_lib, cursor, key, cpu) {
    if (cpu[SURF_CPU_LEVEL]) {
      cpu_Cas01_t CPU = cpu[SURF_CPU_LEVEL];
      xbt_swag_free(CPU->action_set);
    }
  }

  lmm_system_free(cpu_maxmin_system);
  cpu_maxmin_system = NULL;

  surf_model_exit(surf_cpu_model);
  surf_cpu_model = NULL;

  xbt_swag_free(cpu_running_action_set_that_does_not_need_being_checked);
  cpu_running_action_set_that_does_not_need_being_checked = NULL;
  if (cpu_action_heap)
    xbt_heap_free(cpu_action_heap);
  if (cpu_modified_cpu)
    xbt_swag_free(cpu_modified_cpu);
}

static void surf_cpu_model_init_internal()
{
  s_surf_action_t action;
  s_cpu_Cas01_t cpu;

  surf_cpu_model = surf_model_init();

  cpu_running_action_set_that_does_not_need_being_checked =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));

  surf_cpu_model->name = "cpu";

  surf_cpu_model->action_unref = cpu_action_unref;
  surf_cpu_model->action_cancel = cpu_action_cancel;
  surf_cpu_model->action_state_set = cpu_cpu_action_state_set;

  surf_cpu_model->model_private->resource_used = cpu_resource_used;

  if (cpu_update_mechanism == UM_LAZY) {
    surf_cpu_model->model_private->share_resources =
        cpu_share_resources_lazy;
    surf_cpu_model->model_private->update_actions_state =
        cpu_update_actions_state_lazy;
  } else if (cpu_update_mechanism == UM_FULL) {
    surf_cpu_model->model_private->share_resources =
        cpu_share_resources_full;
    surf_cpu_model->model_private->update_actions_state =
        cpu_update_actions_state_full;
  } else
    xbt_die("Invalid update mechanism!");

  surf_cpu_model->model_private->update_resource_state =
      cpu_update_resource_state;
  surf_cpu_model->model_private->finalize = cpu_finalize;

  surf_cpu_model->suspend = cpu_action_suspend;
  surf_cpu_model->resume = cpu_action_resume;
  surf_cpu_model->is_suspended = cpu_action_is_suspended;
  surf_cpu_model->set_max_duration = cpu_action_set_max_duration;
  surf_cpu_model->set_priority = cpu_action_set_priority;
#ifdef HAVE_TRACING
  surf_cpu_model->set_category = cpu_action_set_category;
#endif
  surf_cpu_model->get_remains = cpu_action_get_remains;

  surf_cpu_model->extension.cpu.execute = cpu_execute;
  surf_cpu_model->extension.cpu.sleep = cpu_action_sleep;

  surf_cpu_model->extension.cpu.get_state = cpu_get_state;
  surf_cpu_model->extension.cpu.get_speed = cpu_get_speed;
  surf_cpu_model->extension.cpu.get_available_speed =
      cpu_get_available_speed;
  surf_cpu_model->extension.cpu.create_resource = cpu_create_resource;
  surf_cpu_model->extension.cpu.add_traces = cpu_add_traces_cpu;

  if (!cpu_maxmin_system) {
    cpu_maxmin_system = lmm_system_new(cpu_selective_update);
  }
  if (cpu_update_mechanism == UM_LAZY) {
    cpu_action_heap = xbt_heap_new(8, NULL);
    xbt_heap_set_update_callback(cpu_action_heap,
                                 cpu_action_update_index_heap);
    cpu_modified_cpu =
        xbt_swag_new(xbt_swag_offset(cpu, modified_cpu_hookup));
  }
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

void surf_cpu_model_init_Cas01()
{
  char *optim = xbt_cfg_get_string(_surf_cfg_set, "cpu/optim");
  int select =
      xbt_cfg_get_int(_surf_cfg_set, "cpu/maxmin_selective_update");

  if (!strcmp(optim, "Full")) {
    cpu_update_mechanism = UM_FULL;
    cpu_selective_update = select;
  } else if (!strcmp(optim, "Lazy")) {
    cpu_update_mechanism = UM_LAZY;
    cpu_selective_update = 1;
    xbt_assert((select == 1)
               ||
               (xbt_cfg_is_default_value
                (_surf_cfg_set, "cpu/maxmin_selective_update")),
               "Disabling selective update while using the lazy update mechanism is dumb!");
  } else if (!strcmp(optim, "TI")) {
    surf_cpu_model_init_ti();
    return;
  } else {
    xbt_die("Unsupported optimization (%s) for this model", optim);
  }

  if (surf_cpu_model)
    return;
  surf_cpu_model_init_internal();
  cpu_define_callbacks();
  xbt_dynar_push(model_list, &surf_cpu_model);
}
