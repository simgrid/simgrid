
/* Copyright (c) 2009, 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/*
  commit: e2d6799c4182f00443b3013aadb1c2412372460f
  This commit retrieves the old implementation of CPU_TI with multi-levels.
*/

#include "surf_private.h"
#include "trace_mgr_private.h"
#include "cpu_ti_private.h"
#include "xbt/heap.h"
#include "surf/surf_resource.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_cpu_ti, surf,
                                "Logging specific to the SURF CPU TRACE INTEGRATION module");


static xbt_swag_t
    cpu_ti_running_action_set_that_does_not_need_being_checked = NULL;
static xbt_swag_t cpu_ti_modified_cpu = NULL;
static xbt_heap_t cpu_ti_action_heap;

/* prototypes of new trace functions */
static double surf_cpu_ti_integrate_trace(surf_cpu_ti_tgmr_t trace,
                                          double a, double b);


static double surf_cpu_ti_solve_trace(surf_cpu_ti_tgmr_t trace, double a,
                                      double amount);
static double surf_cpu_ti_solve_trace_somewhat_simple(surf_cpu_ti_tgmr_t
                                                      trace, double a,
                                                      double amount);

static void surf_cpu_ti_free_tmgr(surf_cpu_ti_tgmr_t trace);

static double surf_cpu_ti_integrate_trace_simple(surf_cpu_ti_trace_t trace,
                                                 double a, double b);
static double surf_cpu_ti_integrate_trace_simple_point(surf_cpu_ti_trace_t
                                                       trace, double a);
static double surf_cpu_ti_solve_trace_simple(surf_cpu_ti_trace_t trace,
                                             double a, double amount);
static int surf_cpu_ti_binary_search(double *array, double a, int low,
                                     int high);
/* end prototypes */

static void surf_cpu_ti_free_trace(surf_cpu_ti_trace_t trace)
{
  xbt_free(trace->time_points);
  xbt_free(trace->integral);
  xbt_free(trace);
}

static void surf_cpu_ti_free_tmgr(surf_cpu_ti_tgmr_t trace)
{
  if (trace->trace)
    surf_cpu_ti_free_trace(trace->trace);
  xbt_free(trace);
}

static surf_cpu_ti_trace_t surf_cpu_ti_trace_new(tmgr_trace_t power_trace)
{
  surf_cpu_ti_trace_t trace;
  s_tmgr_event_t val;
  unsigned int cpt;
  double integral = 0;
  double time = 0;
  int i = 0;
  trace = xbt_new0(s_surf_cpu_ti_trace_t, 1);
  trace->time_points =
      xbt_malloc0(sizeof(double) *
                  (xbt_dynar_length(power_trace->s_list.event_list) + 1));
  trace->integral =
      xbt_malloc0(sizeof(double) *
                  (xbt_dynar_length(power_trace->s_list.event_list) + 1));
  trace->nb_points = xbt_dynar_length(power_trace->s_list.event_list);
  xbt_dynar_foreach(power_trace->s_list.event_list, cpt, val) {
    trace->time_points[i] = time;
    trace->integral[i] = integral;
    integral += val.delta * val.value;
    time += val.delta;
    i++;
  }
  trace->time_points[i] = time;
  trace->integral[i] = integral;
  return trace;
}

/**
* \brief Creates a new integration trace from a tmgr_trace_t
*
* \param  power_trace    CPU availability trace
* \param  value          Percentage of CPU power available (useful to fixed tracing)
* \param  spacing        Initial spacing
* \return  Integration trace structure
*/
static surf_cpu_ti_tgmr_t cpu_ti_parse_trace(tmgr_trace_t power_trace,
                                             double value)
{
  surf_cpu_ti_tgmr_t trace;
  double total_time = 0.0;
  s_tmgr_event_t val;
  unsigned int cpt;
  trace = xbt_new0(s_surf_cpu_ti_tgmr_t, 1);

/* no availability file, fixed trace */
  if (!power_trace) {
    trace->type = TRACE_FIXED;
    trace->value = value;
    XBT_DEBUG("No availabily trace. Constant value = %lf", value);
    return trace;
  }

  /* only one point available, fixed trace */
  if (xbt_dynar_length(power_trace->s_list.event_list) == 1) {
    xbt_dynar_get_cpy(power_trace->s_list.event_list, 0, &val);
    trace->type = TRACE_FIXED;
    trace->value = val.value;
    return trace;
  }

  trace->type = TRACE_DYNAMIC;
  trace->power_trace = power_trace;

  /* count the total time of trace file */
  xbt_dynar_foreach(power_trace->s_list.event_list, cpt, val) {
    total_time += val.delta;
  }
  trace->trace = surf_cpu_ti_trace_new(power_trace);
  trace->last_time = total_time;
  trace->total =
      surf_cpu_ti_integrate_trace_simple(trace->trace, 0, total_time);

  XBT_DEBUG("Total integral %lf, last_time %lf ",
         trace->total, trace->last_time);

  return trace;
}


static void* cpu_ti_create_resource(const char *name, double power_peak,
                           double power_scale,
                           tmgr_trace_t power_trace,
                           int core,
                           e_surf_resource_state_t state_initial,
                           tmgr_trace_t state_trace,
                           xbt_dict_t cpu_properties,
                           surf_model_t cpu_model)
{
  tmgr_trace_t empty_trace;
  s_tmgr_event_t val;
  cpu_ti_t cpu = NULL;
  s_surf_action_cpu_ti_t ti_action;
  xbt_assert(core==1,"Multi-core not handled with this model yet");
  xbt_assert(!surf_cpu_resource_priv(surf_cpu_resource_by_name(name)),
              "Host '%s' declared several times in the platform file",
              name);
  xbt_assert(core==1,"Multi-core not handled with this model yet");
  cpu = (cpu_ti_t) surf_resource_new(sizeof(s_cpu_ti_t),
          cpu_model, name,cpu_properties);
  cpu->action_set =
      xbt_swag_new(xbt_swag_offset(ti_action, cpu_list_hookup));
  cpu->power_peak = power_peak;
  xbt_assert(cpu->power_peak > 0, "Power has to be >0");
  XBT_DEBUG("power scale %lf", power_scale);
  cpu->power_scale = power_scale;
  cpu->avail_trace = cpu_ti_parse_trace(power_trace, power_scale);
  cpu->state_current = state_initial;
  if (state_trace)
    cpu->state_event =
        tmgr_history_add_trace(history, state_trace, 0.0, 0, cpu);
  if (power_trace && xbt_dynar_length(power_trace->s_list.event_list) > 1) {
    /* add a fake trace event if periodicity == 0 */
    xbt_dynar_get_cpy(power_trace->s_list.event_list,
                      xbt_dynar_length(power_trace->s_list.event_list) - 1, &val);
    if (val.delta == 0) {
      empty_trace = tmgr_empty_trace_new();
      cpu->power_event =
          tmgr_history_add_trace(history, empty_trace,
                                 cpu->avail_trace->last_time, 0, cpu);
    }
  }
  xbt_lib_set(host_lib, name, SURF_CPU_LEVEL, cpu);

  return xbt_lib_get_elm_or_null(host_lib, name);
}


static void parse_cpu_ti_init(sg_platf_host_cbarg_t host)
{
  cpu_ti_create_resource(host->id,
        host->power_peak,
        host->power_scale,
        host->power_trace,
        host->core_amount,
        host->initial_state,
        host->state_trace,
        host->properties,
        surf_cpu_model_pm);

}

static void add_traces_cpu_ti(void)
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
    cpu_ti_t cpu = surf_cpu_resource_priv(surf_cpu_resource_by_name(elm));

    xbt_assert(cpu, "Host %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    if (cpu->state_event) {
      XBT_DEBUG("Trace already configured for this CPU(%s), ignoring it",
             elm);
      continue;
    }
    XBT_DEBUG("Add state trace: %s to CPU(%s)", trace_name, elm);
    cpu->state_event = tmgr_history_add_trace(history, trace, 0.0, 0, cpu);
  }

  xbt_dict_foreach(trace_connect_list_power, cursor, trace_name, elm) {
    tmgr_trace_t trace = xbt_dict_get_or_null(traces_set_list, trace_name);
    cpu_ti_t cpu = surf_cpu_resource_priv(surf_cpu_resource_by_name(elm));

    xbt_assert(cpu, "Host %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    XBT_DEBUG("Add power trace: %s to CPU(%s)", trace_name, elm);
    if (cpu->avail_trace)
      surf_cpu_ti_free_tmgr(cpu->avail_trace);

    cpu->avail_trace = cpu_ti_parse_trace(trace, cpu->power_scale);

    /* add a fake trace event if periodicity == 0 */
    if (trace && xbt_dynar_length(trace->s_list.event_list) > 1) {
      s_tmgr_event_t val;
      xbt_dynar_get_cpy(trace->s_list.event_list,
                        xbt_dynar_length(trace->s_list.event_list) - 1, &val);
      if (val.delta == 0) {
        tmgr_trace_t empty_trace;
        empty_trace = tmgr_empty_trace_new();
        cpu->power_event =
            tmgr_history_add_trace(history, empty_trace,
                                   cpu->avail_trace->last_time, 0, cpu);
      }
    }
  }
}

static void cpu_ti_define_callbacks()
{
  sg_platf_host_add_cb(parse_cpu_ti_init);
  sg_platf_postparse_add_cb(add_traces_cpu_ti);
}

static int cpu_ti_resource_used(void *resource_id)
{
  cpu_ti_t cpu = resource_id;
  return xbt_swag_size(cpu->action_set);
}

static int cpu_ti_action_unref(surf_action_t action)
{
  action->refcount--;
  if (!action->refcount) {
    xbt_swag_remove(action, action->state_set);
    /* remove from action_set */
    xbt_swag_remove(action, ((cpu_ti_t)surf_cpu_resource_priv(ACTION_GET_CPU(action)))->action_set);
    /* remove from heap */
    xbt_heap_remove(cpu_ti_action_heap,
                    ((surf_action_cpu_ti_t) action)->index_heap);
    xbt_swag_insert(((cpu_ti_t)surf_cpu_resource_priv(ACTION_GET_CPU(action))), cpu_ti_modified_cpu);
    surf_action_free(&action);
    return 1;
  }
  return 0;
}

static void cpu_ti_action_cancel(surf_action_t action)
{
  surf_action_state_set(action, SURF_ACTION_FAILED);
  xbt_heap_remove(cpu_ti_action_heap,
                  ((surf_action_cpu_ti_t) action)->index_heap);
  xbt_swag_insert(surf_cpu_resource_priv(ACTION_GET_CPU(action)), cpu_ti_modified_cpu);
  return;
}

static void cpu_ti_action_state_set(surf_action_t action,
                                    e_surf_action_state_t state)
{
  surf_action_state_set(action, state);
  xbt_swag_insert(surf_cpu_resource_priv(ACTION_GET_CPU(action)), cpu_ti_modified_cpu);
  return;
}

/**
* \brief Update the remaining amount of actions
*
* \param  cpu    Cpu on which the actions are running
* \param  now    Current time
*/
static void cpu_ti_update_remaining_amount(surf_model_t cpu_model, cpu_ti_t cpu, double now)
{
  double area_total;
  surf_action_cpu_ti_t action;

/* already updated */
  if (cpu->last_update >= now)
    return;

/* calcule the surface */
  area_total =
      surf_cpu_ti_integrate_trace(cpu->avail_trace, cpu->last_update,
                                  now) * cpu->power_peak;
  XBT_DEBUG("Flops total: %lf, Last update %lf", area_total,
         cpu->last_update);

  xbt_swag_foreach(action, cpu->action_set) {
    surf_action_t generic = (surf_action_t)action;
    /* action not running, skip it */
    if (generic->state_set !=
        cpu_model->states.running_action_set)
      continue;

    /* bogus priority, skip it */
    if (generic->priority <= 0)
      continue;

    /* action suspended, skip it */
    if (action->suspended != 0)
      continue;

    /* action don't need update */
    if (generic->start >= now)
      continue;

    /* skip action that are finishing now */
    if (generic->finish >= 0
        && generic->finish <= now)
      continue;

    /* update remaining */
    double_update(&(generic->remains),
                  area_total / (cpu->sum_priority *
                                generic->priority));
    XBT_DEBUG("Update remaining action(%p) remaining %lf", action,
           generic->remains);
  }
  cpu->last_update = now;
#undef GENERIC_ACTION
}

/**
* \brief Update the finish date of action if necessary
*
* \param  cpu    Cpu on which the actions are running
* \param  now    Current time
*/
static void cpu_ti_update_action_finish_date(surf_model_t cpu_model, cpu_ti_t cpu, double now)
{
#define GENERIC_ACTION(action) action->generic_action
  surf_action_cpu_ti_t action;
  double sum_priority = 0.0, total_area, min_finish = -1;

/* update remaning amount of actions */
  cpu_ti_update_remaining_amount(cpu_model, cpu, now);

  xbt_swag_foreach(action, cpu->action_set) {
    /* action not running, skip it */
    if (GENERIC_ACTION(action).state_set !=
        cpu_model->states.running_action_set)
      continue;

    /* bogus priority, skip it */
    if (GENERIC_ACTION(action).priority <= 0)
      continue;

    /* action suspended, skip it */
    if (action->suspended != 0)
      continue;

    sum_priority += 1.0 / GENERIC_ACTION(action).priority;
  }
  cpu->sum_priority = sum_priority;

  xbt_swag_foreach(action, cpu->action_set) {
    min_finish = -1;
    /* action not running, skip it */
    if (GENERIC_ACTION(action).state_set !=
        cpu_model->states.running_action_set)
      continue;

    /* verify if the action is really running on cpu */
    if (action->suspended == 0 && GENERIC_ACTION(action).priority > 0) {
      /* total area needed to finish the action. Used in trace integration */
      total_area =
          (GENERIC_ACTION(action).remains) * sum_priority *
          GENERIC_ACTION(action).priority;

      total_area /= cpu->power_peak;

      GENERIC_ACTION(action).finish =
          surf_cpu_ti_solve_trace(cpu->avail_trace, now, total_area);
      /* verify which event will happen before (max_duration or finish time) */
      if ((GENERIC_ACTION(action).max_duration != NO_MAX_DURATION) &&
          (GENERIC_ACTION(action).start +
           GENERIC_ACTION(action).max_duration <
           GENERIC_ACTION(action).finish))
        min_finish = GENERIC_ACTION(action).start +
            GENERIC_ACTION(action).max_duration;
      else
        min_finish = GENERIC_ACTION(action).finish;
    } else {
      /* put the max duration time on heap */
      if (GENERIC_ACTION(action).max_duration != NO_MAX_DURATION)
        min_finish =
            (GENERIC_ACTION(action).start +
             GENERIC_ACTION(action).max_duration);
    }
    /* add in action heap */
    XBT_DEBUG("action(%p) index %d", action, action->index_heap);
    if (action->index_heap >= 0) {
      surf_action_cpu_ti_t heap_act =
          xbt_heap_remove(cpu_ti_action_heap, action->index_heap);
      if (heap_act != action)
        DIE_IMPOSSIBLE;
    }
    if (min_finish != NO_MAX_DURATION)
      xbt_heap_push(cpu_ti_action_heap, action, min_finish);

    XBT_DEBUG
        ("Update finish time: Cpu(%s) Action: %p, Start Time: %lf Finish Time: %lf Max duration %lf",
         cpu->generic_resource.name, action, GENERIC_ACTION(action).start,
         GENERIC_ACTION(action).finish,
         GENERIC_ACTION(action).max_duration);
  }
/* remove from modified cpu */
  xbt_swag_remove(cpu, cpu_ti_modified_cpu);
#undef GENERIC_ACTION
}

static double cpu_ti_share_resources(surf_model_t cpu_model, double now)
{
  cpu_ti_t cpu, cpu_next;
  double min_action_duration = -1;

/* iterates over modified cpus to update share resources */
  xbt_swag_foreach_safe(cpu, cpu_next, cpu_ti_modified_cpu) {
    /* FIXME: cpu_ti_modified_cpu is a global object. But, now we have multiple
     * cpu_model objects in the system. Do we have to create this
     * swag for each cpu model object?
     *
     * We should revisit here after we know what cpu_ti is.
     **/
    cpu_ti_update_action_finish_date(cpu_model, cpu, now);
  }
/* get the min next event if heap not empty */
  if (xbt_heap_size(cpu_ti_action_heap) > 0)
    min_action_duration = xbt_heap_maxkey(cpu_ti_action_heap) - now;

  XBT_DEBUG("Share resources, min next event date: %lf", min_action_duration);

  return min_action_duration;
}

static void cpu_ti_update_actions_state(surf_model_t cpu_model, double now, double delta)
{
  /* FIXME: cpu_ti_action_heap is global. Is this okay for VM support? */

#define GENERIC_ACTION(action) action->generic_action
  surf_action_cpu_ti_t action;
  while ((xbt_heap_size(cpu_ti_action_heap) > 0)
         && (xbt_heap_maxkey(cpu_ti_action_heap) <= now)) {
    action = xbt_heap_pop(cpu_ti_action_heap);
    XBT_DEBUG("Action %p: finish", action);
    GENERIC_ACTION(action).finish = surf_get_clock();
    /* set the remains to 0 due to precision problems when updating the remaining amount */
    GENERIC_ACTION(action).remains = 0;
    cpu_ti_action_state_set((surf_action_t) action, SURF_ACTION_DONE);
    /* update remaining amout of all actions */
    cpu_ti_update_remaining_amount(cpu_model, surf_cpu_resource_priv(action->cpu), surf_get_clock());
  }
#undef GENERIC_ACTION
}

static void cpu_ti_update_resource_state(void *id,
                                         tmgr_trace_event_t event_type,
                                         double value, double date)
{
  cpu_ti_t cpu = id;
  surf_model_t cpu_model = ((surf_resource_t) cpu)->model;
  surf_action_cpu_ti_t action;

  surf_watched_hosts();

  if (event_type == cpu->power_event) {
    tmgr_trace_t power_trace;
    surf_cpu_ti_tgmr_t trace;
    s_tmgr_event_t val;

    XBT_DEBUG("Finish trace date: %lf value %lf date %lf", surf_get_clock(),
           value, date);
    /* update remaining of actions and put in modified cpu swag */
    cpu_ti_update_remaining_amount(cpu_model, cpu, date);
    xbt_swag_insert(cpu, cpu_ti_modified_cpu);

    power_trace = cpu->avail_trace->power_trace;
    xbt_dynar_get_cpy(power_trace->s_list.event_list,
                      xbt_dynar_length(power_trace->s_list.event_list) - 1, &val);
    /* free old trace */
    surf_cpu_ti_free_tmgr(cpu->avail_trace);
    cpu->power_scale = val.value;

    trace = xbt_new0(s_surf_cpu_ti_tgmr_t, 1);
    trace->type = TRACE_FIXED;
    trace->value = val.value;
    XBT_DEBUG("value %lf", val.value);

    cpu->avail_trace = trace;

    if (tmgr_trace_event_free(event_type))
      cpu->power_event = NULL;

  } else if (event_type == cpu->state_event) {
    if (value > 0)
      cpu->state_current = SURF_RESOURCE_ON;
    else {
      cpu->state_current = SURF_RESOURCE_OFF;

      /* put all action running on cpu to failed */
      xbt_swag_foreach(action, cpu->action_set) {
        if (surf_action_state_get((surf_action_t) action) ==
            SURF_ACTION_RUNNING
            || surf_action_state_get((surf_action_t) action) ==
            SURF_ACTION_READY
            || surf_action_state_get((surf_action_t) action) ==
            SURF_ACTION_NOT_IN_THE_SYSTEM) {
          action->generic_action.finish = date;
          cpu_ti_action_state_set((surf_action_t) action,
                                  SURF_ACTION_FAILED);
          if (action->index_heap >= 0) {
            surf_action_cpu_ti_t heap_act =
                xbt_heap_remove(cpu_ti_action_heap, action->index_heap);
            if (heap_act != action)
              DIE_IMPOSSIBLE;
          }
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

static surf_action_t cpu_ti_execute(void *cpu, double size)
{
  surf_action_cpu_ti_t action = NULL;
  cpu_ti_t CPU = surf_cpu_resource_priv(cpu);
  surf_model_t cpu_model = ((surf_resource_t) CPU)->model;

  XBT_IN("(%s,%g)", surf_resource_name(CPU), size);
  action =
      surf_action_new(sizeof(s_surf_action_cpu_ti_t), size, cpu_model,
                      CPU->state_current != SURF_RESOURCE_ON);
  action->cpu = cpu;
  action->index_heap = -1;

  xbt_swag_insert(CPU, cpu_ti_modified_cpu);

  xbt_swag_insert(action, CPU->action_set);

  action->suspended = 0;        /* Should be useless because of the
                                   calloc but it seems to help valgrind... */

  XBT_OUT();
  return (surf_action_t) action;
}

static void cpu_ti_action_update_index_heap(void *action, int i)
{
  ((surf_action_cpu_ti_t) action)->index_heap = i;
}

static surf_action_t cpu_ti_action_sleep(void *cpu, double duration)
{
  surf_action_cpu_ti_t action = NULL;

  if (duration > 0)
    duration = MAX(duration, MAXMIN_PRECISION);

  XBT_IN("(%s,%g)", surf_resource_name(surf_cpu_resource_priv(cpu)), duration);
  action = (surf_action_cpu_ti_t) cpu_ti_execute(cpu, 1.0);
  action->generic_action.max_duration = duration;
  action->suspended = 2;
  if (duration == NO_MAX_DURATION) {
    /* Move to the *end* of the corresponding action set. This convention
       is used to speed up update_resource_state  */
    xbt_swag_remove(action, ((surf_action_t) action)->state_set);
    ((surf_action_t) action)->state_set =
        cpu_ti_running_action_set_that_does_not_need_being_checked;
    xbt_swag_insert(action, ((surf_action_t) action)->state_set);
  }
  XBT_OUT();
  return (surf_action_t) action;
}

static void cpu_ti_action_suspend(surf_action_t action)
{
  XBT_IN("(%p)", action);
  if (((surf_action_cpu_ti_t) action)->suspended != 2) {
    ((surf_action_cpu_ti_t) action)->suspended = 1;
    xbt_heap_remove(cpu_ti_action_heap,
                    ((surf_action_cpu_ti_t) action)->index_heap);
    xbt_swag_insert(surf_cpu_resource_priv(ACTION_GET_CPU(action)), cpu_ti_modified_cpu);
  }
  XBT_OUT();
}

static void cpu_ti_action_resume(surf_action_t action)
{
  XBT_IN("(%p)", action);
  if (((surf_action_cpu_ti_t) action)->suspended != 2) {
    ((surf_action_cpu_ti_t) action)->suspended = 0;
    xbt_swag_insert(surf_cpu_resource_priv(ACTION_GET_CPU(action)), cpu_ti_modified_cpu);
  }
  XBT_OUT();
}

static int cpu_ti_action_is_suspended(surf_action_t action)
{
  return (((surf_action_cpu_ti_t) action)->suspended == 1);
}

static void cpu_ti_action_set_max_duration(surf_action_t action,
                                           double duration)
{
  surf_action_cpu_ti_t ACT = (surf_action_cpu_ti_t) action;
  double min_finish;

  XBT_IN("(%p,%g)", action, duration);

  action->max_duration = duration;

  if (duration >= 0)
    min_finish =
        (action->start + action->max_duration) <
        action->finish ? (action->start +
                          action->max_duration) : action->finish;
  else
    min_finish = action->finish;

/* add in action heap */
  if (ACT->index_heap >= 0) {
    surf_action_cpu_ti_t heap_act =
        xbt_heap_remove(cpu_ti_action_heap, ACT->index_heap);
    if (heap_act != ACT)
      DIE_IMPOSSIBLE;
  }
  xbt_heap_push(cpu_ti_action_heap, ACT, min_finish);

  XBT_OUT();
}

static void cpu_ti_action_set_priority(surf_action_t action,
                                       double priority)
{
  XBT_IN("(%p,%g)", action, priority);
  action->priority = priority;
  xbt_swag_insert(surf_cpu_resource_priv(ACTION_GET_CPU(action)), cpu_ti_modified_cpu);
  XBT_OUT();
}

static double cpu_ti_action_get_remains(surf_action_t action)
{
  surf_model_t cpu_model = action->model_obj;
  XBT_IN("(%p)", action);

  cpu_ti_update_remaining_amount(cpu_model, (cpu_ti_t) ((surf_action_cpu_ti_t) action)->cpu,
                                 surf_get_clock());
  XBT_OUT();
  return action->remains;
}

static e_surf_resource_state_t cpu_ti_get_state(void *cpu)
{
  return ((cpu_ti_t)surf_cpu_resource_priv(cpu))->state_current;
}

static double cpu_ti_get_speed(void *cpu, double load)
{
  return load * ((cpu_ti_t)surf_cpu_resource_priv(cpu))->power_peak;
}

/**
* \brief Auxiliary function to update the CPU power scale.
*
*  This function uses the trace structure to return the power scale at the determined time a.
* \param trace    Trace structure to search the updated power scale
* \param a        Time
* \return CPU power scale
*/
static double surf_cpu_ti_get_power_scale(surf_cpu_ti_tgmr_t trace,
                                          double a)
{
  double reduced_a;
  int point;
  s_tmgr_event_t val;

  reduced_a = a - floor(a / trace->last_time) * trace->last_time;
  point =
      surf_cpu_ti_binary_search(trace->trace->time_points, reduced_a, 0,
                                trace->trace->nb_points - 1);
  xbt_dynar_get_cpy(trace->power_trace->s_list.event_list, point, &val);
  return val.value;
}

static double cpu_ti_get_available_speed(void *cpu)
{
  cpu_ti_t CPU = surf_cpu_resource_priv(cpu);
  CPU->power_scale =
      surf_cpu_ti_get_power_scale(CPU->avail_trace, surf_get_clock());
/* number between 0 and 1 */
  return CPU->power_scale;
}

static void cpu_ti_finalize(surf_model_t cpu_model)
{
  void **cpu;
  xbt_lib_cursor_t cursor;
  char *key;

  /* FIXME: we should update this code for VM support */
  xbt_abort();

  xbt_lib_foreach(host_lib, cursor, key, cpu){
    if(cpu[SURF_CPU_LEVEL])
    {
        cpu_ti_t CPU = cpu[SURF_CPU_LEVEL];
        xbt_swag_free(CPU->action_set);
        surf_cpu_ti_free_tmgr(CPU->avail_trace);
    }
  }

  surf_model_exit(cpu_model);
  cpu_model = NULL;

  xbt_swag_free
      (cpu_ti_running_action_set_that_does_not_need_being_checked);
  xbt_swag_free(cpu_ti_modified_cpu);
  cpu_ti_running_action_set_that_does_not_need_being_checked = NULL;
  xbt_heap_free(cpu_ti_action_heap);
}

surf_model_t surf_cpu_ti_model_init_internal(void)
{
  s_surf_action_t action;
  s_cpu_ti_t cpu;

  surf_model_t cpu_model = surf_model_init();

  cpu_ti_running_action_set_that_does_not_need_being_checked =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));

  cpu_ti_modified_cpu =
      xbt_swag_new(xbt_swag_offset(cpu, modified_cpu_hookup));

  cpu_model->name = "cpu_ti";
  cpu_model->type = SURF_MODEL_TYPE_CPU;

  cpu_model->action_unref = cpu_ti_action_unref;
  cpu_model->action_cancel = cpu_ti_action_cancel;
  cpu_model->action_state_set = cpu_ti_action_state_set;

  cpu_model->model_private->resource_used = cpu_ti_resource_used;
  cpu_model->model_private->share_resources = cpu_ti_share_resources;
  cpu_model->model_private->update_actions_state =
      cpu_ti_update_actions_state;
  cpu_model->model_private->update_resource_state =
      cpu_ti_update_resource_state;
  cpu_model->model_private->finalize = cpu_ti_finalize;

  cpu_model->suspend = cpu_ti_action_suspend;
  cpu_model->resume = cpu_ti_action_resume;
  cpu_model->is_suspended = cpu_ti_action_is_suspended;
  cpu_model->set_max_duration = cpu_ti_action_set_max_duration;
  cpu_model->set_priority = cpu_ti_action_set_priority;
  cpu_model->get_remains = cpu_ti_action_get_remains;

  cpu_model->extension.cpu.execute = cpu_ti_execute;
  cpu_model->extension.cpu.sleep = cpu_ti_action_sleep;

  cpu_model->extension.cpu.get_state = cpu_ti_get_state;
  cpu_model->extension.cpu.get_speed = cpu_ti_get_speed;
  cpu_model->extension.cpu.get_available_speed =
      cpu_ti_get_available_speed;
  cpu_model->extension.cpu.add_traces = add_traces_cpu_ti;

  cpu_ti_action_heap = xbt_heap_new(8, NULL);
  xbt_heap_set_update_callback(cpu_ti_action_heap,
                               cpu_ti_action_update_index_heap);

  return cpu_model;
}

surf_model_t surf_cpu_model_init_ti(void)
{
  surf_model_t cpu_model = surf_cpu_ti_model_init_internal();
  cpu_ti_define_callbacks();
  xbt_dynar_push(model_list, &cpu_model);

  return cpu_model;
}


/**
* \brief Integrate trace
*
* Wrapper around surf_cpu_integrate_trace_simple() to get
* the cyclic effect.
*
* \param trace Trace structure.
* \param a      Begin of interval
* \param b      End of interval
* \return the integrate value. -1 if an error occurs.
*/
static double surf_cpu_ti_integrate_trace(surf_cpu_ti_tgmr_t trace,
                                          double a, double b)
{
  double first_chunk;
  double middle_chunk;
  double last_chunk;
  int a_index, b_index;

  if ((a < 0.0) || (a > b)) {
    XBT_CRITICAL
        ("Error, invalid integration interval [%.2f,%.2f]. You probably have a task executing with negative computation amount. Check your code.",
         a, b);
    xbt_abort();
  }
  if (a == b)
    return 0.0;

  if (trace->type == TRACE_FIXED) {
    return ((b - a) * trace->value);
  }

  if (ceil(a / trace->last_time) == a / trace->last_time)
    a_index = 1 + (int) (ceil(a / trace->last_time));
  else
    a_index = (int) (ceil(a / trace->last_time));

  b_index = (int) (floor(b / trace->last_time));

  if (a_index > b_index) {      /* Same chunk */
    return surf_cpu_ti_integrate_trace_simple(trace->trace,
                                              a - (a_index -
                                                   1) * trace->last_time,
                                              b -
                                              (b_index) *
                                              trace->last_time);
  }

  first_chunk = surf_cpu_ti_integrate_trace_simple(trace->trace,
                                                   a - (a_index -
                                                        1) *
                                                   trace->last_time,
                                                   trace->last_time);
  middle_chunk = (b_index - a_index) * trace->total;
  last_chunk = surf_cpu_ti_integrate_trace_simple(trace->trace,
                                                  0.0,
                                                  b -
                                                  (b_index) *
                                                  trace->last_time);

  XBT_DEBUG("first_chunk=%.2f  middle_chunk=%.2f  last_chunk=%.2f\n",
         first_chunk, middle_chunk, last_chunk);

  return (first_chunk + middle_chunk + last_chunk);
}

/**
 * \brief Auxiliary function to calculate the integral between a and b.
 *     It simply calculates the integral at point a and b and returns the difference 
 *   between them.
 * \param trace    Trace structure
 * \param a        Initial point
 * \param b  Final point
 * \return  Integral
*/
static double surf_cpu_ti_integrate_trace_simple(surf_cpu_ti_trace_t trace,
                                                 double a, double b)
{
  return surf_cpu_ti_integrate_trace_simple_point(trace,
                                                  b) -
      surf_cpu_ti_integrate_trace_simple_point(trace, a);
}

/**
 * \brief Auxiliary function to calculate the integral at point a.
 * \param trace    Trace structure
 * \param a        point
 * \return  Integral
*/
static double surf_cpu_ti_integrate_trace_simple_point(surf_cpu_ti_trace_t
                                                       trace, double a)
{
  double integral = 0;
  int ind;
  double a_aux = a;
  ind =
      surf_cpu_ti_binary_search(trace->time_points, a, 0,
                                trace->nb_points - 1);
  integral += trace->integral[ind];
  XBT_DEBUG
      ("a %lf ind %d integral %lf ind + 1 %lf ind %lf time +1 %lf time %lf",
       a, ind, integral, trace->integral[ind + 1], trace->integral[ind],
       trace->time_points[ind + 1], trace->time_points[ind]);
  double_update(&a_aux, trace->time_points[ind]);
  if (a_aux > 0)
    integral +=
        ((trace->integral[ind + 1] -
          trace->integral[ind]) / (trace->time_points[ind + 1] -
                                   trace->time_points[ind])) * (a -
                                                                trace->
                                                                time_points
                                                                [ind]);
  XBT_DEBUG("Integral a %lf = %lf", a, integral);

  return integral;
}

/**
* \brief Calculate the time needed to execute "amount" on cpu.
*
* Here, amount can span multiple trace periods
*
* \param trace   CPU trace structure
* \param a        Initial time
* \param amount  Amount to be executed
* \return  End time
*/
static double surf_cpu_ti_solve_trace(surf_cpu_ti_tgmr_t trace, double a,
                                      double amount)
{
  int quotient;
  double reduced_b;
  double reduced_amount;
  double reduced_a;
  double b;

/* Fix very small negative numbers */
  if ((a < 0.0) && (a > -EPSILON)) {
    a = 0.0;
  }
  if ((amount < 0.0) && (amount > -EPSILON)) {
    amount = 0.0;
  }

/* Sanity checks */
  if ((a < 0.0) || (amount < 0.0)) {
    XBT_CRITICAL
        ("Error, invalid parameters [a = %.2f, amount = %.2f]. You probably have a task executing with negative computation amount. Check your code.",
         a, amount);
    xbt_abort();
  }

/* At this point, a and amount are positive */

  if (amount < EPSILON)
    return a;

/* Is the trace fixed ? */
  if (trace->type == TRACE_FIXED) {
    return (a + (amount / trace->value));
  }

  XBT_DEBUG("amount %lf total %lf", amount, trace->total);
/* Reduce the problem to one where amount <= trace_total */
  quotient = (int) (floor(amount / trace->total));
  reduced_amount = (trace->total) * ((amount / trace->total) -
                                     floor(amount / trace->total));
  reduced_a = a - (trace->last_time) * (int) (floor(a / trace->last_time));

  XBT_DEBUG("Quotient: %d reduced_amount: %lf reduced_a: %lf", quotient,
         reduced_amount, reduced_a);

/* Now solve for new_amount which is <= trace_total */
/*
   fprintf(stderr,"reduced_a = %.2f\n",reduced_a);
   fprintf(stderr,"reduced_amount = %.2f\n",reduced_amount);
 */
  reduced_b =
      surf_cpu_ti_solve_trace_somewhat_simple(trace, reduced_a,
                                              reduced_amount);

/* Re-map to the original b and amount */
  b = (trace->last_time) * (int) (floor(a / trace->last_time)) +
      (quotient * trace->last_time) + reduced_b;
  return b;
}

/**
* \brief Auxiliary function to solve integral
*
* Here, amount is <= trace->total
* and a <=trace->last_time
*
*/
static double surf_cpu_ti_solve_trace_somewhat_simple(surf_cpu_ti_tgmr_t
                                                      trace, double a,
                                                      double amount)
{
  double amount_till_end;
  double b;

  XBT_DEBUG("Solve integral: [%.2f, amount=%.2f]", a, amount);
  amount_till_end =
      surf_cpu_ti_integrate_trace(trace, a, trace->last_time);
/*
   fprintf(stderr,"amount_till_end=%.2f\n",amount_till_end);
 */

  if (amount_till_end > amount) {
    b = surf_cpu_ti_solve_trace_simple(trace->trace, a, amount);
  } else {
    b = trace->last_time +
        surf_cpu_ti_solve_trace_simple(trace->trace, 0.0,
                                       amount - amount_till_end);
  }
  return b;
}

/**
 * \brief Auxiliary function to solve integral.
 *  It returns the date when the requested amount of flops is available
 * \param trace    Trace structure
 * \param a        Initial point
 * \param amount  Amount of flops 
 * \return The date when amount is available.
*/
static double surf_cpu_ti_solve_trace_simple(surf_cpu_ti_trace_t trace,
                                             double a, double amount)
{
  double integral_a;
  int ind;
  double time;
  integral_a = surf_cpu_ti_integrate_trace_simple_point(trace, a);
  ind =
      surf_cpu_ti_binary_search(trace->integral, integral_a + amount, 0,
                                trace->nb_points - 1);
  time = trace->time_points[ind];
  time +=
      (integral_a + amount -
       trace->integral[ind]) / ((trace->integral[ind + 1] -
                                 trace->integral[ind]) /
                                (trace->time_points[ind + 1] -
                                 trace->time_points[ind]));

  return time;
}

/**
 * \brief Binary search in array.
 *  It returns the first point of the interval in which "a" is. 
 * \param array    Array
 * \param a        Value to search
 * \param low     Low bound to search in array
 * \param high    Upper bound to search in array
 * \return Index of point
*/
static int surf_cpu_ti_binary_search(double *array, double a, int low,
                                     int high)
{
  xbt_assert(low < high, "Wrong parameters: low (%d) should be smaller than"
      " high (%d)", low, high);

  int mid;
  do {
    mid = low + (high - low) / 2;
    XBT_DEBUG("a %lf low %d high %d mid %d value %lf", a, low, high, mid,
        array[mid]);

    if (array[mid] > a)
      high = mid;
    else
      low = mid;
  }
  while (low < high - 1);

  return low;
}
