
/* 	$Id$	 */

/* Copyright (c) 2004 Arnaud Legrand. All rights reserved.                  */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_private.h"
#include "trace_mgr_private.h"
#include "cpu_ti_private.h"
#include "xbt/heap.h"


XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_cpu_ti, surf,
                                "Logging specific to the SURF CPU TRACE INTEGRATION module");


static xbt_swag_t running_action_set_that_does_not_need_being_checked = NULL;
static xbt_swag_t modified_cpu = NULL;
static xbt_heap_t action_heap;

/* prototypes of new trace functions */
static double surf_cpu_integrate_trace(surf_cpu_ti_tgmr_t trace, double a,
                                       double b);
static double surf_cpu_integrate_trace_simple(surf_cpu_ti_tgmr_t trace,
                                              double a, double b);


static double surf_cpu_solve_trace(surf_cpu_ti_tgmr_t trace, double a,
                                   double amount);
static double surf_cpu_solve_trace_somewhat_simple(surf_cpu_ti_tgmr_t trace,
                                                   double a, double amount);
static double surf_cpu_solve_trace_simple(surf_cpu_ti_tgmr_t trace, double a,
                                          double amount);

static void surf_cpu_free_trace(surf_cpu_ti_tgmr_t trace);
static void surf_cpu_free_time_series(surf_cpu_ti_timeSeries_t timeSeries);
static double surf_cpu_integrate_exactly(surf_cpu_ti_tgmr_t trace, int index,
                                         double a, double b);
static double surf_cpu_solve_exactly(surf_cpu_ti_tgmr_t trace, int index,
                                     double a, double amount);
/* end prototypes */

static void surf_cpu_free_time_series(surf_cpu_ti_timeSeries_t timeSeries)
{
  xbt_free(timeSeries->values);
  xbt_free(timeSeries->trace_index);
  xbt_free(timeSeries->trace_value);
  xbt_free(timeSeries);
}

static void surf_cpu_free_trace(surf_cpu_ti_tgmr_t trace)
{
  int i;

  for (i = 0; i < trace->nb_levels; i++)
    surf_cpu_free_time_series(trace->levels[i]);

  xbt_free(trace->levels);
  xbt_free(trace);
}

static surf_cpu_ti_timeSeries_t surf_cpu_ti_time_series_new(tmgr_trace_t
                                                            power_trace,
                                                            double spacing,
                                                            double total_time)
{
  surf_cpu_ti_timeSeries_t series;
  double time = 0.0, value = 0.0, sum_delta = 0.0;
  double previous_time = 0.0, old_time = 0.0;
  int event_lost = 0, next_index = 0;
  double next_time = 0.0;
  int total_alloc = 0;
  s_tmgr_event_t val;
  unsigned int cpt;

  series = xbt_new0(s_surf_cpu_ti_timeSeries_t, 1);
  series->spacing = spacing;
  series->power_trace = power_trace;

  /* alloc memory only once, it's faster than doing realloc each time */
  total_alloc = (int) ceil((total_time / spacing));
  series->values = xbt_malloc0(sizeof(double) * total_alloc);
  series->trace_index = xbt_malloc0(sizeof(int) * total_alloc);
  series->trace_value = xbt_malloc0(sizeof(double) * total_alloc);

/* FIXME: it doesn't work with traces with only one point and periodicity = 0
 * if the trace is always cyclic and periodicity must be > 0 it works */
  xbt_dynar_foreach(power_trace->event_list, cpt, val) {
    /* delta = the next trace event
     * value = state until next event */
    time += val.delta;

    /* ignore events until next spacing */
    if (time < (series->nb_points + 1) * spacing) {
      value += val.value * val.delta;
      sum_delta += val.delta;
      old_time = time;
      event_lost = 1;
      continue;
    }

    /* update value and sum_delta with the space between last point in trace(old_time) and next spacing */
    value += val.value * ((series->nb_points + 1) * spacing - old_time);
    sum_delta += ((series->nb_points + 1) * spacing - old_time);
    /* calcule the value to next spacing */
    value /= sum_delta;

    while (previous_time + spacing <= time) {
      /* update first spacing with mean of points.
       * others with the right value */
      if (event_lost) {
        series->values[series->nb_points] = value;
        series->trace_index[series->nb_points] = next_index;
        series->trace_value[series->nb_points] = next_time;
        event_lost = 0;
      } else {
        series->values[series->nb_points] = val.value;
        series->trace_index[series->nb_points] = -1;
      }
      (series->nb_points)++;
      previous_time += spacing;
    }
    /* update value and sum_delta, interval: [time, next spacing] */
    value = (time - previous_time) * val.value;
    sum_delta = (time - previous_time);
    old_time = time;
    next_index = (int) cpt + 1;
    next_time = time;
    event_lost = 1;
  }
  /* last spacing
   * ignore small amount at end */
  double_update(&time, (series->nb_points) * spacing);
  if (time > 0) {
    /* here the value is divided by spacing in order to have exactly the value of last chunk (independently of its size). After, when we calcule the integral, we multiply by actual_last_time insted of last_time */
    value /= spacing;
    series->values[series->nb_points] = value;
    /* update first spacing with mean of points
     * others with the right value */
    if (event_lost) {
      series->trace_index[series->nb_points] = next_index;
      series->trace_value[series->nb_points] = next_time;
    } else {
      series->trace_index[series->nb_points] = -1;
    }
    (series->nb_points)++;
  }
  return series;
}

/**
* \brief Create new levels of points.
*
* This function assumes that the input series is
* evenly spaces, starting at time 0. That is the sort
* of series produced by surf_cpu_ti_time_series_new()
*
* \param	original	Original timeSeries structure
* \param	factor		New factor to spacing
* \return					New timeSeries structure with spacing*factor
*/
static surf_cpu_ti_timeSeries_t
surf_cpu_ti_time_series_coarsen(surf_cpu_ti_timeSeries_t original, int factor)
{
  surf_cpu_ti_timeSeries_t series;
  int j, i = 0;
  double dfactor = (double) (factor);
  double ave;

  if (original->nb_points <= factor) {
    DEBUG0("Warning: Not enough data points to coarsen time series");
    return NULL;
  }

  series = xbt_new0(s_surf_cpu_ti_timeSeries_t, 1);
  series->spacing = (original->spacing) * dfactor;

  while (i + factor <= original->nb_points) {
    /* Averaging */
    ave = 0.0;
    for (j = i; j < i + factor; j++) {
      ave += original->values[j];
    }
    ave /= dfactor;
    /* Updating */
    series->values = xbt_realloc(series->values,
                                 (series->nb_points + 1) * sizeof(double));
    series->values[(series->nb_points)++] = ave;
    i += factor;
  }

  return series;
}

/**
* \brief Create a new integration trace from a tmgr_trace_t
*
* \param	power_trace		CPU availability trace
* \param	value					Percentage of CPU power disponible (usefull to fixed tracing)
* \param	spacing				Initial spacing
* \return	Integration trace structure
*/
static surf_cpu_ti_tgmr_t cpu_ti_parse_trace(tmgr_trace_t power_trace,
                                             double value)
{
  surf_cpu_ti_tgmr_t trace;
  surf_cpu_ti_timeSeries_t series;
  int i;
  double total_time = 0.0;
  s_tmgr_event_t val;
  unsigned int cpt;
  trace = xbt_new0(s_surf_cpu_ti_tgmr_t, 1);

/* no availability file, fixed trace */
  if (!power_trace) {
    trace->type = TRACE_FIXED;
    trace->value = value;
    DEBUG1("No availabily trace. Constant value = %lf", value);
    return trace;
  }

  /* count the total time of trace file */
  xbt_dynar_foreach(power_trace->event_list, cpt, val) {
    total_time += val.delta;
  }
  trace->actual_last_time = total_time;

  DEBUG2("Value %lf, Spacing %lf", value, power_trace->timestep);
  series =
    surf_cpu_ti_time_series_new(power_trace, power_trace->timestep,
                                total_time);
  if (!series)
    return NULL;

  trace->type = TRACE_DYNAMIC;

  trace->levels = xbt_new0(surf_cpu_ti_timeSeries_t, 1);
  trace->levels[(trace->nb_levels)++] = series;

/* Do the coarsening with some arbitrary factors */
  for (i = 1; i < TRACE_NB_LEVELS; i++) {
    series = surf_cpu_ti_time_series_coarsen(trace->levels[i - 1], 4 * i);

    if (series) {               /* If coarsening was possible, add it */
      trace->levels = xbt_realloc(trace->levels,
                                  (trace->nb_levels +
                                   1) * sizeof(s_surf_cpu_ti_timeSeries_t));
      trace->levels[(trace->nb_levels)++] = series;
    } else {                    /* otherwise stop */
      break;
    }
  }
/* calcul of initial integrate */
  trace->last_time =
    power_trace->timestep * ((double) (trace->levels[0]->nb_points));

  trace->total = surf_cpu_integrate_trace(trace, 0.0, trace->last_time);

  DEBUG3("Total integral %lf, last_time %lf actual_last_time %lf",
         trace->total, trace->last_time, trace->actual_last_time);

  return trace;
}


static cpu_ti_t cpu_new(char *name, double power_peak,
                        double power_scale,
                        tmgr_trace_t power_trace,
                        e_surf_resource_state_t state_initial,
                        tmgr_trace_t state_trace, xbt_dict_t cpu_properties)
{
  cpu_ti_t cpu = xbt_new0(s_cpu_ti_t, 1);
  s_surf_action_cpu_ti_t ti_action;
  xbt_assert1(!surf_model_resource_by_name(surf_cpu_model, name),
              "Host '%s' declared several times in the platform file", name);
  cpu->action_set = xbt_swag_new(xbt_swag_offset(ti_action, cpu_list_hookup));
  cpu->generic_resource.model = surf_cpu_model;
  cpu->generic_resource.name = name;
  cpu->generic_resource.properties = cpu_properties;
  cpu->power_peak = power_peak;
  xbt_assert0(cpu->power_peak > 0, "Power has to be >0");
  DEBUG1("power scale %lf", power_scale);
  cpu->power_scale = power_scale;
  cpu->avail_trace = cpu_ti_parse_trace(power_trace, power_scale);
  cpu->state_current = state_initial;
  if (state_trace)
    cpu->state_event =
      tmgr_history_add_trace(history, state_trace, 0.0, 0, cpu);

  xbt_dict_set(surf_model_resource_set(surf_cpu_model), name, cpu,
               surf_resource_free);

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
    cpu_ti_t cpu = surf_model_resource_by_name(surf_cpu_model, elm);

    xbt_assert1(cpu, "Host %s undefined", elm);
    xbt_assert1(trace, "Trace %s undefined", trace_name);

    if (cpu->state_event) {
      DEBUG1("Trace already configured for this CPU(%s), ignoring it", elm);
      continue;
    }
    DEBUG2("Add state trace: %s to CPU(%s)", trace_name, elm);
    cpu->state_event = tmgr_history_add_trace(history, trace, 0.0, 0, cpu);
  }

  xbt_dict_foreach(trace_connect_list_power, cursor, trace_name, elm) {
    tmgr_trace_t trace = xbt_dict_get_or_null(traces_set_list, trace_name);
    cpu_ti_t cpu = surf_model_resource_by_name(surf_cpu_model, elm);

    xbt_assert1(cpu, "Host %s undefined", elm);
    xbt_assert1(trace, "Trace %s undefined", trace_name);

    DEBUG2("Add power trace: %s to CPU(%s)", trace_name, elm);
    if (cpu->avail_trace)
      surf_cpu_free_trace(cpu->avail_trace);

    cpu->avail_trace = cpu_ti_parse_trace(trace, cpu->power_scale);
  }
}

static void define_callbacks(const char *file)
{
  surf_parse_reset_parser();
  surfxml_add_callback(STag_surfxml_host_cb_list, parse_cpu_init);
  surfxml_add_callback(ETag_surfxml_platform_cb_list, &add_traces_cpu);
}

static int resource_used(void *resource_id)
{
  cpu_ti_t cpu = resource_id;
  return xbt_swag_size(cpu->action_set);
}

static int action_unref(surf_action_t action)
{
  action->refcount--;
  if (!action->refcount) {
    xbt_swag_remove(action, action->state_set);
    /* remove from action_set */
    xbt_swag_remove(action, ACTION_GET_CPU(action)->action_set);
    /* remove from heap */
    xbt_heap_remove(action_heap, ((surf_action_cpu_ti_t) action)->index_heap);
    xbt_swag_insert(ACTION_GET_CPU(action), modified_cpu);
    free(action);
    return 1;
  }
  return 0;
}

static void action_cancel(surf_action_t action)
{
  surf_action_state_set(action, SURF_ACTION_FAILED);
  xbt_heap_remove(action_heap, ((surf_action_cpu_ti_t) action)->index_heap);
  xbt_swag_insert(ACTION_GET_CPU(action), modified_cpu);
  return;
}

static void cpu_action_state_set(surf_action_t action,
                                 e_surf_action_state_t state)
{
  surf_action_state_set(action, state);
  xbt_swag_insert(ACTION_GET_CPU(action), modified_cpu);
  return;
}

/**
* \brief Update the remaning amount of actions
*
* \param	cpu		Cpu on which the actions are running
* \param	now		Current time
*/
static void cpu_update_remaining_amount(cpu_ti_t cpu, double now)
{
#define GENERIC_ACTION(action) action->generic_action
  double area_total;
  surf_action_cpu_ti_t action;

/* alrealdy updated */
  if (cpu->last_update == now)
    return;

/* calcule the surface */
  area_total =
    surf_cpu_integrate_trace(cpu->avail_trace, cpu->last_update,
                             now) * cpu->power_peak;
  DEBUG2("Flops total: %lf, Last update %lf", area_total, cpu->last_update);

  xbt_swag_foreach(action, cpu->action_set) {
    /* action not running, skip it */
    if (GENERIC_ACTION(action).state_set !=
        surf_cpu_model->states.running_action_set)
      continue;

    /* bogus priority, skip it */
    if (GENERIC_ACTION(action).priority <= 0)
      continue;

    /* action suspended, skip it */
    if (action->suspended != 0)
      continue;

    /* action don't need update */
    if (GENERIC_ACTION(action).start >= now)
      continue;

    /* skip action that are finishing now */
    if (GENERIC_ACTION(action).finish >= 0
        && GENERIC_ACTION(action).finish <= now)
      continue;

    /* update remaining */
    double_update(&(GENERIC_ACTION(action).remains),
                  area_total / (cpu->sum_priority *
                                GENERIC_ACTION(action).priority));
    DEBUG2("Update remaining action(%p) remaining %lf", action,
           GENERIC_ACTION(action).remains);
  }
  cpu->last_update = now;
#undef GENERIC_ACTION
}

/**
* \brief Update the finish date of action if necessary
*
* \param	cpu		Cpu on which the actions are running
* \param	now		Current time
*/
static void cpu_update_action_finish_date(cpu_ti_t cpu, double now)
{
#define GENERIC_ACTION(action) action->generic_action
  surf_action_cpu_ti_t action;
  double sum_priority = 0.0, total_area, min_finish = -1;

/* update remaning amount of actions */
  cpu_update_remaining_amount(cpu, now);

  xbt_swag_foreach(action, cpu->action_set) {
    /* action not running, skip it */
    if (GENERIC_ACTION(action).state_set !=
        surf_cpu_model->states.running_action_set)
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
    /* action not running, skip it */
    if (GENERIC_ACTION(action).state_set !=
        surf_cpu_model->states.running_action_set)
      continue;

    /* verify if the action is really running on cpu */
    if (action->suspended == 0 && GENERIC_ACTION(action).priority > 0) {
      /* total area needed to finish the action. Used in trace integration */
      total_area =
        (GENERIC_ACTION(action).remains) * sum_priority *
        GENERIC_ACTION(action).priority;

      total_area /= cpu->power_peak;

      GENERIC_ACTION(action).finish =
        surf_cpu_solve_trace(cpu->avail_trace, now, total_area);
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
    DEBUG2("action(%p) index %d", action, action->index_heap);
    if (action->index_heap >= 0) {
      surf_action_cpu_ti_t heap_act =
        xbt_heap_remove(action_heap, action->index_heap);
      if (heap_act != action)
        DIE_IMPOSSIBLE;
    }
    if (min_finish != NO_MAX_DURATION)
      xbt_heap_push(action_heap, action, min_finish);

    DEBUG5
      ("Update finish time: Cpu(%s) Action: %p, Start Time: %lf Finish Time: %lf Max duration %lf",
       cpu->generic_resource.name, action, GENERIC_ACTION(action).start,
       GENERIC_ACTION(action).finish, GENERIC_ACTION(action).max_duration);
  }
/* remove from modified cpu */
  xbt_swag_remove(cpu, modified_cpu);
#undef GENERIC_ACTION
}

static double share_resources(double now)
{
  cpu_ti_t cpu, cpu_next;
  double min_action_duration = -1;

/* iterates over modified cpus to update share resources */
  xbt_swag_foreach_safe(cpu, cpu_next, modified_cpu) {
    cpu_update_action_finish_date(cpu, now);
  }
/* get the min next event if heap not empty */
  if (xbt_heap_size(action_heap) > 0)
    min_action_duration = xbt_heap_maxkey(action_heap) - now;

  DEBUG1("Share resources, min next event date: %lf", min_action_duration);

  return min_action_duration;
}

static void update_actions_state(double now, double delta)
{
#define GENERIC_ACTION(action) action->generic_action
  surf_action_cpu_ti_t action;
  while ((xbt_heap_size(action_heap) > 0)
         && (xbt_heap_maxkey(action_heap) <= now)) {
    DEBUG1("Action %p: finish", action);
    action = xbt_heap_pop(action_heap);
    GENERIC_ACTION(action).finish = surf_get_clock();
    /* set the remains to 0 due to precision problems when updating the remaining amount */
    GENERIC_ACTION(action).remains = 0;
    cpu_action_state_set((surf_action_t) action, SURF_ACTION_DONE);
    /* update remaining amout of all actions */
    cpu_update_remaining_amount(action->cpu, surf_get_clock());
  }
#undef GENERIC_ACTION
}

static void update_resource_state(void *id,
                                  tmgr_trace_event_t event_type,
                                  double value, double date)
{
  cpu_ti_t cpu = id;
  surf_action_cpu_ti_t action;
  if (event_type == cpu->state_event) {
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
          cpu_action_state_set((surf_action_t) action, SURF_ACTION_FAILED);
          if (action->index_heap >= 0) {
            surf_action_cpu_ti_t heap_act =
              xbt_heap_remove(action_heap, action->index_heap);
            if (heap_act != action)
              DIE_IMPOSSIBLE;
          }
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

static surf_action_t execute(void *cpu, double size)
{
  surf_action_cpu_ti_t action = NULL;
  cpu_ti_t CPU = cpu;

  XBT_IN2("(%s,%g)", surf_resource_name(CPU), size);
  action =
    surf_action_new(sizeof(s_surf_action_cpu_ti_t), size, surf_cpu_model,
                    CPU->state_current != SURF_RESOURCE_ON);
  action->cpu = cpu;
  action->index_heap = -1;

  xbt_swag_insert(CPU, modified_cpu);

  xbt_swag_insert(action, CPU->action_set);

  action->suspended = 0;        /* Should be useless because of the
                                   calloc but it seems to help valgrind... */

  XBT_OUT;
  return (surf_action_t) action;
}

static void action_update_index_heap(void *action, int i)
{
  ((surf_action_cpu_ti_t) action)->index_heap = i;
}

static surf_action_t action_sleep(void *cpu, double duration)
{
  surf_action_cpu_ti_t action = NULL;

  if (duration > 0)
    duration = MAX(duration, MAXMIN_PRECISION);

  XBT_IN2("(%s,%g)", surf_resource_name(cpu), duration);
  action = (surf_action_cpu_ti_t) execute(cpu, 1.0);
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
  XBT_OUT;
  return (surf_action_t) action;
}

static void action_suspend(surf_action_t action)
{
  XBT_IN1("(%p)", action);
  if (((surf_action_cpu_ti_t) action)->suspended != 2) {
    ((surf_action_cpu_ti_t) action)->suspended = 1;
    xbt_swag_insert(ACTION_GET_CPU(action), modified_cpu);
  }
  XBT_OUT;
}

static void action_resume(surf_action_t action)
{
  XBT_IN1("(%p)", action);
  if (((surf_action_cpu_ti_t) action)->suspended != 2) {
    ((surf_action_cpu_ti_t) action)->suspended = 0;
    xbt_swag_insert(ACTION_GET_CPU(action), modified_cpu);
  }
  XBT_OUT;
}

static int action_is_suspended(surf_action_t action)
{
  return (((surf_action_cpu_ti_t) action)->suspended == 1);
}

static void action_set_max_duration(surf_action_t action, double duration)
{
  surf_action_cpu_ti_t ACT = (surf_action_cpu_ti_t) action;
  double min_finish;

  XBT_IN2("(%p,%g)", action, duration);

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
      xbt_heap_remove(action_heap, ACT->index_heap);
    if (heap_act != ACT)
      DIE_IMPOSSIBLE;
  }
  xbt_heap_push(action_heap, ACT, min_finish);

  XBT_OUT;
}

static void action_set_priority(surf_action_t action, double priority)
{
  XBT_IN2("(%p,%g)", action, priority);
  action->priority = priority;
  xbt_swag_insert(ACTION_GET_CPU(action), modified_cpu);
  XBT_OUT;
}

static double action_get_remains(surf_action_t action)
{
  XBT_IN1("(%p)", action);
  cpu_update_remaining_amount((cpu_ti_t) ((surf_action_cpu_ti_t) action)->cpu,
                              surf_get_clock());
  return action->remains;
  XBT_OUT;
}

static e_surf_resource_state_t get_state(void *cpu)
{
  return ((cpu_ti_t) cpu)->state_current;
}

static double get_speed(void *cpu, double load)
{
  return load * (((cpu_ti_t) cpu)->power_peak);
}

/**
* \brief Auxiliar function to update the cpu power scale.
*
*	This function uses the trace structure to return the power scale at the determined time a.
* \param trace		Trace structure to search the updated power scale
* \param a				Time
* \return Cpu power scale
*/
static double surf_cpu_get_power_scale(surf_cpu_ti_tgmr_t trace, double a)
{
  double reduced_a;
  int point;

  reduced_a = a - floor(a / trace->last_time) * trace->last_time;
  point = (int) (reduced_a / trace->levels[0]->spacing);
  return trace->levels[0]->values[point];
}

static double get_available_speed(void *cpu)
{
  cpu_ti_t CPU = cpu;
  CPU->power_scale =
    surf_cpu_get_power_scale(CPU->avail_trace, surf_get_clock());
/* number between 0 and 1 */
  return CPU->power_scale;
}

static void finalize(void)
{
  void *cpu;
  xbt_dict_cursor_t cursor;
  char *key;
  xbt_dict_foreach(surf_model_resource_set(surf_cpu_model), cursor, key, cpu) {
    cpu_ti_t CPU = cpu;
    xbt_swag_free(CPU->action_set);
    surf_cpu_free_trace(CPU->avail_trace);
  }

  surf_model_exit(surf_cpu_model);
  surf_cpu_model = NULL;

  xbt_swag_free(running_action_set_that_does_not_need_being_checked);
  xbt_swag_free(modified_cpu);
  running_action_set_that_does_not_need_being_checked = NULL;
  xbt_heap_free(action_heap);
}

static void surf_cpu_model_init_internal(void)
{
  s_surf_action_t action;
  s_cpu_ti_t cpu;

  surf_cpu_model = surf_model_init();

  running_action_set_that_does_not_need_being_checked =
    xbt_swag_new(xbt_swag_offset(action, state_hookup));

  modified_cpu = xbt_swag_new(xbt_swag_offset(cpu, modified_cpu_hookup));

  surf_cpu_model->name = "CPU_TI";

  surf_cpu_model->action_unref = action_unref;
  surf_cpu_model->action_cancel = action_cancel;
  surf_cpu_model->action_state_set = cpu_action_state_set;

  surf_cpu_model->model_private->resource_used = resource_used;
  surf_cpu_model->model_private->share_resources = share_resources;
  surf_cpu_model->model_private->update_actions_state = update_actions_state;
  surf_cpu_model->model_private->update_resource_state =
    update_resource_state;
  surf_cpu_model->model_private->finalize = finalize;

  surf_cpu_model->suspend = action_suspend;
  surf_cpu_model->resume = action_resume;
  surf_cpu_model->is_suspended = action_is_suspended;
  surf_cpu_model->set_max_duration = action_set_max_duration;
  surf_cpu_model->set_priority = action_set_priority;
  surf_cpu_model->get_remains = action_get_remains;

  surf_cpu_model->extension.cpu.execute = execute;
  surf_cpu_model->extension.cpu.sleep = action_sleep;

  surf_cpu_model->extension.cpu.get_state = get_state;
  surf_cpu_model->extension.cpu.get_speed = get_speed;
  surf_cpu_model->extension.cpu.get_available_speed = get_available_speed;

  action_heap = xbt_heap_new(8, NULL);
  xbt_heap_set_update_callback(action_heap, action_update_index_heap);

}

void surf_cpu_model_init_ti(const char *filename)
{
  if (surf_cpu_model)
    return;
  surf_cpu_model_init_internal();
  define_callbacks(filename);
  xbt_dynar_push(model_list, &surf_cpu_model);
}


///////////////// BEGIN INTEGRAL //////////////

/**
* \brief Integrate trace
*
* Wrapper around surf_cpu_integrate_trace_simple() to get
* the cyclic effect.
*
* \param trace Trace structure.
* \param a			Begin of interval
* \param b			End of interval
* \return the integrate value. -1 if an error occurs.
*/
static double surf_cpu_integrate_trace(surf_cpu_ti_tgmr_t trace, double a,
                                       double b)
{
  double first_chunk;
  double middle_chunk;
  double last_chunk;
  int a_index, b_index;

  if ((a < 0.0) || (a > b)) {
    CRITICAL2
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
    return surf_cpu_integrate_trace_simple(trace,
                                           a - (a_index -
                                                1) * trace->last_time,
                                           b - (b_index) * trace->last_time);
  }

  first_chunk = surf_cpu_integrate_trace_simple(trace,
                                                a - (a_index -
                                                     1) * trace->last_time,
                                                trace->last_time);
  middle_chunk = (b_index - a_index) * trace->total;
  last_chunk = surf_cpu_integrate_trace_simple(trace,
                                               0.0,
                                               b -
                                               (b_index) * trace->last_time);

  DEBUG3("first_chunk=%.2f  middle_chunk=%.2f  last_chunk=%.2f\n",
         first_chunk, middle_chunk, last_chunk);

  return (first_chunk + middle_chunk + last_chunk);
}

/**
* \brief Integrate the trace between a and b.
*
*  integrates without taking cyclic-traces into account.
*  [a,b] \subset [0,last_time]
*
* \param trace Trace structure.
* \param a			Begin of interval
* \param b			End of interval
* \return the integrate value. -1 if an error occurs.
*/
static double surf_cpu_integrate_trace_simple(surf_cpu_ti_tgmr_t trace,
                                              double a, double b)
{
  double integral = 0.0;
  int i;
  long index;
  int top_level = 0;
  long l_bounds[TRACE_NB_LEVELS];
  long u_bounds[TRACE_NB_LEVELS];
  double a_divided_by_spacing;
  double current_spacing;
  DEBUG2("Computing simple integral on [%.2f , %.2f]\n", a, b);

/* Sanity checks */
  if ((a < 0.0) || (b < a) || (a > trace->last_time)
      || (b > trace->last_time)) {
    CRITICAL2
      ("Error, invalid integration interval [%.2f,%.2f]. You probably have a task executing with negative computation amount. Check your code.",
       a, b);
    xbt_abort();
  }
  if (b == a) {
    return 0.0;
  }

  for (i = 0; i < trace->nb_levels; i++) {
    a_divided_by_spacing = a / trace->levels[i]->spacing;
    if (ceil(a_divided_by_spacing) == a_divided_by_spacing)
      l_bounds[i] = 1 + (long) ceil(a_divided_by_spacing);
    else
      l_bounds[i] = (long) (ceil(a_divided_by_spacing));
    if (b == trace->last_time) {
      u_bounds[i] = (long) (floor(b / trace->levels[i]->spacing)) - 1;
    } else {
      u_bounds[i] = (long) (floor(b / trace->levels[i]->spacing));
    }
    DEBUG3("level %d: l%ld  u%ld\n", i, l_bounds[i], u_bounds[i]);

    if (l_bounds[i] <= u_bounds[i])
      top_level = i;
  }
  DEBUG1("top_level=%d\n", top_level);

/* Are a and b BOTH in the same chunk of level 0 ? */
  if (l_bounds[0] > u_bounds[0]) {
    return (b - a) * (trace->levels[0]->values[u_bounds[0]]);
  }

/* first sub-level amount */
  integral += ((l_bounds[0]) * (trace->levels[0]->spacing) - a) *
    (trace->levels[0]->values[l_bounds[0] - 1]);

  DEBUG1("Initial level 0 amount is %.2f\n", integral);

/* first n-1 levels */
  for (i = 0; i < top_level; i++) {

    if (l_bounds[i] >= u_bounds[i])
      break;

    current_spacing = trace->levels[i]->spacing;
    index = l_bounds[i];

    DEBUG1("L%d:", i);

    while (double_positive
           (l_bounds[i + 1] * trace->levels[i + 1]->spacing -
            index * current_spacing)) {
      integral += current_spacing * trace->levels[i]->values[index];
      DEBUG2("%.2f->%.2f|",
             index * (trace->levels[i]->spacing),
             (index + 1) * (trace->levels[i]->spacing));
      index++;
    }

    DEBUG0("\n");
  }

  DEBUG1("After going up: %.2f\n", integral);

/* n-th level */
  current_spacing = trace->levels[top_level]->spacing;
  index = l_bounds[top_level];

  DEBUG1("L%d:", top_level);

  while (index < u_bounds[top_level]) {
    integral += current_spacing * trace->levels[top_level]->values[index];

    DEBUG2("%.2f->%.2f|",
           index * (trace->levels[top_level]->spacing),
           (index + 1) * (trace->levels[top_level]->spacing));

    index++;
  }

  DEBUG0("\n");
  DEBUG1("After steady : %.2f\n", integral);

/* And going back down */
  for (i = top_level - 1; i >= 0; i--) {
    if (l_bounds[i] > u_bounds[i])
      break;

    current_spacing = trace->levels[i]->spacing;
    index = u_bounds[i + 1] * (trace->levels[i + 1]->spacing /
                               current_spacing);
    DEBUG1("L%d:", i);
    while (double_positive
           ((u_bounds[i]) * current_spacing - index * current_spacing)) {
      integral += current_spacing * trace->levels[i]->values[index];
      DEBUG2("%.2f->%.2f|",
             index * (trace->levels[i]->spacing),
             (index + 1) * (trace->levels[i]->spacing));
      index++;
    }
  }

  DEBUG1("After going down : %.2f", integral);


/* Little piece at the end */
  integral += (b - u_bounds[0] * (trace->levels[0]->spacing)) *
    (trace->levels[0]->values[u_bounds[0]]);
  DEBUG1("After last bit : %.2f", integral);

  return integral;
}


/**
* \brief Calcul the time needed to execute "amount" on cpu.
*
* Here, amount can span multiple trace periods
*
* \param trace 	CPU trace structure
* \param a				Initial time
* \param amount	Amount of calcul to be executed
* \return	End time
*/
static double surf_cpu_solve_trace(surf_cpu_ti_tgmr_t trace, double a,
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
    CRITICAL2
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

  DEBUG2("amount %lf total %lf", amount, trace->total);
/* Reduce the problem to one where amount <= trace_total */
  quotient = (int) (floor(amount / trace->total));
  reduced_amount = (trace->total) * ((amount / trace->total) -
                                     floor(amount / trace->total));
  reduced_a = a - (trace->last_time) * (int) (floor(a / trace->last_time));

  DEBUG3("Quotient: %d reduced_amount: %lf reduced_a: %lf", quotient,
         reduced_amount, reduced_a);

/* Now solve for new_amount which is <= trace_total */
/*
	 fprintf(stderr,"reduced_a = %.2f\n",reduced_a);
	 fprintf(stderr,"reduced_amount = %.2f\n",reduced_amount);
 */
  reduced_b =
    surf_cpu_solve_trace_somewhat_simple(trace, reduced_a, reduced_amount);

/* Re-map to the original b and amount */
  b = (trace->last_time) * (int) (floor(a / trace->last_time)) +
    (quotient * trace->actual_last_time) + reduced_b;
  return b;
}

/**
* \brief Auxiliar function to solve integral
*
* Here, amount is <= trace->total
* and a <=trace->last_time
*
*/
static double surf_cpu_solve_trace_somewhat_simple(surf_cpu_ti_tgmr_t trace,
                                                   double a, double amount)
{
  double amount_till_end;
  double b;

  DEBUG2("Solve integral: [%.2f, amount=%.2f]", a, amount);

  amount_till_end = surf_cpu_integrate_trace(trace, a, trace->last_time);
/*
	 fprintf(stderr,"amount_till_end=%.2f\n",amount_till_end);
 */

  if (amount_till_end > amount) {
    b = surf_cpu_solve_trace_simple(trace, a, amount);
  } else {
    b = trace->last_time +
      surf_cpu_solve_trace_simple(trace, 0.0, amount - amount_till_end);
  }

  return b;
}

/**
* \brief Auxiliar function to solve integral
* surf_cpu_solve_trace_simple()
*
*  solve for the upper bound without taking
*  cyclic-traces into account.
*
*  [a,y] \subset [0,last_time]
*
*/
static double surf_cpu_solve_trace_simple(surf_cpu_ti_tgmr_t trace, double a,
                                          double amount)
{
  double next_chunk;
  double remains;
  int i;
  long index;
  int top_level;
  double b;
  int done;
  long l_bounds[TRACE_NB_LEVELS];       /* May be too bgi for this trace */
  double a_divided_by_spacing;
  double current_spacing;

  DEBUG2("Solving simple integral [x=%.2f,amount=%.2f]", a, amount);

/* Sanity checks */
  if ((a < 0.0) || (amount < 0.0) || (a > trace->last_time)) {
    CRITICAL2
      ("Error, invalid parameters [a = %.2f, amount = %.2f]. You probably have a task executing with negative computation amount. Check your code.",
       a, amount);
    xbt_abort();
  }
  if (amount == 0.0) {
    /* fprintf(stderr,"Warning: trivial integral solve\n"); */
    return a;
  }

  for (i = 0; i < trace->nb_levels; i++) {
    a_divided_by_spacing = a / trace->levels[i]->spacing;
    if (ceil(a_divided_by_spacing) == a_divided_by_spacing)
      l_bounds[i] = 1 + (long) ceil(a_divided_by_spacing);
    else
      l_bounds[i] = (long) (ceil(a_divided_by_spacing));

    if ((l_bounds[i] + 1) * trace->levels[i]->spacing > trace->last_time)
      break;

    DEBUG2("level %d: l%ld", i, l_bounds[i]);
  }
  if (i == trace->nb_levels)
    top_level = trace->nb_levels - 1;
  else {
    top_level = i;
  }

  remains = amount;
/* first sub-level amount */
/* old code, keep here for a while */
#if 0
  next_chunk = ((l_bounds[0]) * (trace->levels[0]->spacing) - a) *
    (trace->levels[0]->values[l_bounds[0] - 1]);
#endif
  next_chunk =
    surf_cpu_integrate_exactly(trace, l_bounds[0] - 1, a,
                               (l_bounds[0]) * (trace->levels[0]->spacing));

  if (remains - next_chunk < 0.0) {
#if 0
    b = a + (amount / trace->levels[0]->values[l_bounds[0] - 1]);
#endif
    b = a + surf_cpu_solve_exactly(trace, l_bounds[0] - 1, a, amount);
    DEBUG1("Returning sub-level[0] result %.2f", b);

    return b;
  } else {
    b = (l_bounds[0]) * (trace->levels[0]->spacing);
    remains -= next_chunk;
  }
  DEBUG2("After sub-0 stuff: remains %.2f (b=%.2f)", remains, b);

/* first n-1 levels */
  DEBUG0("Going up levels");

  done = 0;
  for (i = 0; i < top_level; i++) {

    current_spacing = trace->levels[i]->spacing;
    index = l_bounds[i];

    DEBUG1("L%d:", i);

    while (double_positive
           (l_bounds[i + 1] * trace->levels[i + 1]->spacing -
            index * current_spacing)
           && ((index + 1) * (current_spacing) < trace->last_time)) {

      next_chunk = current_spacing * trace->levels[i]->values[index];

      DEBUG3("%.2f next_chunk= %.2f remains=%.2f",
             (index + 1) * (trace->levels[i]->spacing), next_chunk, remains);

      if (remains - next_chunk < 0.0) { /* Too far */
        done = 1;
        break;
      } else {                  /* Keep going */
        DEBUG2("%.2f->%.2f|",
               index * (trace->levels[i]->spacing),
               (index + 1) * (trace->levels[i]->spacing));

        remains -= next_chunk;
        b = (index + 1) * (current_spacing);
      }
      index++;
    }
    if (done) {
      /* found chunk, fix the index to top level */
      i++;
      break;
    }
  }

  DEBUG0("Steady");
  top_level = i;

/* n-th level */
  current_spacing = trace->levels[top_level]->spacing;
  index = l_bounds[top_level];

  DEBUG1("L%d:", top_level);

/* iterate over the last level only if it hasn't found the chunk where the amount is */
  if (!done) {
    while (index < trace->levels[top_level]->nb_points) {
      next_chunk = current_spacing * trace->levels[top_level]->values[index];
      if (remains - next_chunk <= 0.0) {        /* Too far */
        break;
      } else {
        DEBUG2("%.2f->%.2f|",
               index * (trace->levels[top_level]->spacing),
               (index + 1) * (trace->levels[top_level]->spacing));

        remains -= next_chunk;
        b = (index + 1) * (current_spacing);
      }
      index++;
    }
  }
  DEBUG2("remains = %.2f b=%.2f", remains, b);

/* And going back down */
  DEBUG0("Going back down");
  for (i = top_level - 1; i >= 0; i--) {

    current_spacing = trace->levels[i]->spacing;
    /* use rint to trunc index due to precision problems */
    index = rint(b / trace->levels[i]->spacing);

    DEBUG1("L%d:", i);

    while (index < trace->levels[i]->nb_points) {
      next_chunk = current_spacing * trace->levels[i]->values[index];
      DEBUG3("remains %lf nextchu %lf value %lf", remains, next_chunk,
             trace->levels[i]->values[index]);
      if (remains - next_chunk <= 0.0) {        /* Too far */
        break;
      } else {
        DEBUG3("%.2f->%.2f| b %lf",
               index * (current_spacing), (index + 1) * (current_spacing), b);

        remains -= next_chunk;
        b += current_spacing;
      }
      index++;
    }
  }

  DEBUG2("remains = %.2f b=%.2f\n", remains, b);
  DEBUG1("Last bit index=%ld\n", index);

/* Little piece at the end */
#if 0
  b += (remains) / (trace->levels[0]->values[index]);
#endif
  b += surf_cpu_solve_exactly(trace, index, b, remains);
  DEBUG1("Total b %lf", b);
  return b;
}

/**
* \brief This function calcules the exactly value of integral between a and b. It uses directly the tmgr_trace_t strucure.
* It works only if the two points are in the same timestep.
* \param trace	Trace structure
* \param index	Index of timestep where the points are located
* \param a			First point of interval
* \param b			Second point
* \return	the integral value
*/
static double surf_cpu_integrate_exactly(surf_cpu_ti_tgmr_t trace, int index,
                                         double a, double b)
{
  int tmgr_index;
  double integral = 0.0;
  double time = a;
  double tmgr_date;
  s_tmgr_event_t elem;

  tmgr_index = trace->levels[0]->trace_index[index];
  tmgr_date = trace->levels[0]->trace_value[index];

  DEBUG6
    ("Start time: %lf End time: %lf index %d tmgr_index %d tmgr_date %lf value %lf",
     a, b, index, tmgr_index, tmgr_date, trace->levels[0]->values[index]);

  if (tmgr_index < 0)
    return (b - a) * (trace->levels[0]->values[index]);

  while (a > tmgr_date) {
    xbt_dynar_get_cpy(trace->levels[0]->power_trace->event_list, tmgr_index,
                      &elem);
    tmgr_date += elem.delta;
    if (a <= tmgr_date)
      break;
    tmgr_index++;
  }
  /* sum first slice [a, tmgr_date[ */
  if (a < tmgr_date) {
    xbt_dynar_get_cpy(trace->levels[0]->power_trace->event_list,
                      tmgr_index - 1, &elem);
    if (b < tmgr_date) {
      return (b - a) * elem.value;
    }

    integral = (tmgr_date - a) * elem.value;
    time = tmgr_date;
  }

  while (tmgr_index <
         xbt_dynar_length(trace->levels[0]->power_trace->event_list)) {
    xbt_dynar_get_cpy(trace->levels[0]->power_trace->event_list, tmgr_index,
                      &elem);
    if (b <= time + elem.delta) {
      integral += (b - time) * elem.value;
      break;
    }
    integral += elem.delta * elem.value;
    time += elem.delta;
    tmgr_index++;
  }

  return integral;
}

/**
* \brief This function calcules the exactly time needed to compute amount flops. It uses directly the tmgr_trace_t structure.
* It works only if the two points are in the same timestep.
* \param trace	Trace structure
* \param index	Index of timestep where the points are located
* \param a			Start time
* \param amount			Total amount
* \return	number of seconds needed to compute amount flops
*/
static double surf_cpu_solve_exactly(surf_cpu_ti_tgmr_t trace, int index,
                                     double a, double amount)
{
  int tmgr_index;
  double remains;
  double time, tmgr_date;
  double slice_amount;
  s_tmgr_event_t elem;


  tmgr_index = trace->levels[0]->trace_index[index];
  tmgr_date = trace->levels[0]->trace_value[index];
  remains = amount;
  time = a;

  DEBUG6
    ("Start time: %lf Amount: %lf index %d tmgr_index %d tmgr_date %lf value %lf",
     a, amount, index, tmgr_index, tmgr_date,
     trace->levels[0]->values[index]);
  if (tmgr_index < 0)
    return amount / (trace->levels[0]->values[index]);

  while (a > tmgr_date) {
    xbt_dynar_get_cpy(trace->levels[0]->power_trace->event_list, tmgr_index,
                      &elem);
    tmgr_date += elem.delta;
    if (a <= tmgr_date)
      break;
    tmgr_index++;
  }
  /* sum first slice [a, tmgr_date[ */
  if (a < tmgr_date) {
    xbt_dynar_get_cpy(trace->levels[0]->power_trace->event_list,
                      tmgr_index - 1, &elem);
    slice_amount = (tmgr_date - a) * elem.value;
    DEBUG5("slice amount %lf a %lf tmgr_date %lf elem_value %lf delta %lf",
           slice_amount, a, tmgr_date, elem.value, elem.delta);
    if (remains <= slice_amount) {
      return (remains / elem.value);
    }

    remains -= (tmgr_date - a) * elem.value;
    time = tmgr_date;
  }

  while (1) {
    xbt_dynar_get_cpy(trace->levels[0]->power_trace->event_list, tmgr_index,
                      &elem);
    slice_amount = elem.delta * elem.value;
    DEBUG5("slice amount %lf a %lf tmgr_date %lf elem_value %lf delta %lf",
           slice_amount, a, tmgr_date, elem.value, elem.delta);
    if (remains <= slice_amount) {
      time += remains / elem.value;
      break;
    }
    time += elem.delta;
    remains -= elem.delta * elem.value;
    tmgr_index++;
  }

  return time - a;
}

//////////// END INTEGRAL /////////////////
