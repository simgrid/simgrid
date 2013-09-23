/* Copyright (c) 2009-2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "surf_private.h"
#include "surf/surf_resource.h"
#include "maxmin_private.h"
#include "simgrid/sg_config.h"
#include "cpu_cas01_private.h"

#include "string.h"
#include "stdlib.h"

surf_model_t surf_cpu_model = NULL;


XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_cpu, surf,
                                "Logging specific to the SURF CPU IMPROVED module");

static xbt_swag_t
    cpu_running_action_set_that_does_not_need_being_checked = NULL;

/* Additionnal callback function to cleanup some data, called from surf_resource_free */

static void cpu_cas1_cleanup(void* r){
  cpu_Cas01_t cpu = (cpu_Cas01_t)r;
  unsigned int iter;
  xbt_dynar_t power_tuple = NULL;
  xbt_dynar_foreach(cpu->energy->power_range_watts_list, iter, power_tuple)
    xbt_dynar_free(&power_tuple);
  xbt_dynar_free(&cpu->energy->power_range_watts_list);
  xbt_dynar_free(&cpu->power_peak_list);
  xbt_free(cpu->energy);
  return;
}

/* This function is registered as a callback to sg_platf_new_host() and never called directly */
static void *cpu_create_resource(const char *name, xbt_dynar_t power_peak,
								 int pstate,
                                 double power_scale,
                                 tmgr_trace_t power_trace,
                                 int core,
                                 e_surf_resource_state_t state_initial,
                                 tmgr_trace_t state_trace,
                                 xbt_dict_t cpu_properties)
{
  cpu_Cas01_t cpu = NULL;

  xbt_assert(!surf_cpu_resource_priv(surf_cpu_resource_by_name(name)),
             "Host '%s' declared several times in the platform file",
             name);
  cpu = (cpu_Cas01_t) surf_resource_new(sizeof(s_cpu_Cas01_t),
                                        surf_cpu_model, name,
                                        cpu_properties,  &cpu_cas1_cleanup);
  cpu->power_peak = xbt_dynar_get_as(power_peak, pstate, double);
  cpu->power_peak_list = power_peak;
  cpu->pstate = pstate;

  cpu->energy = xbt_new(s_energy_cpu_cas01_t, 1);
  cpu->energy->total_energy = 0;
  cpu->energy->power_range_watts_list = cpu_get_watts_range_list(cpu);
  cpu->energy->last_updated = surf_get_clock();

  XBT_DEBUG("CPU create: peak=%lf, pstate=%d",cpu->power_peak, cpu->pstate);

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
      lmm_constraint_new(surf_cpu_model->model_private->maxmin_system, cpu,
                         cpu->core * cpu->power_scale * cpu->power_peak);

  xbt_lib_set(host_lib, name, SURF_CPU_LEVEL, cpu);

  return xbt_lib_get_elm_or_null(host_lib, name);;
}


static void parse_cpu_init(sg_platf_host_cbarg_t host)
{
  cpu_create_resource(host->id,
                      host->power_peak,
                      host->pstate,
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
  return lmm_constraint_used(surf_cpu_model->model_private->maxmin_system,
                             ((cpu_Cas01_t) resource)->constraint);
}

static double cpu_share_resources_lazy(double now)
{
  return generic_share_resources_lazy(now, surf_cpu_model);
}

static double cpu_share_resources_full(double now)
{
  s_surf_action_cpu_Cas01_t action;
  return generic_maxmin_share_resources(surf_cpu_model->states.
                                        running_action_set,
                                        xbt_swag_offset(action,
                                                        generic_lmm_action.
                                                        variable),
                                        surf_cpu_model->model_private->maxmin_system, lmm_solve);
}

static void cpu_update_actions_state_lazy(double now, double delta)
{
  generic_update_actions_state_lazy(now, delta, surf_cpu_model);
}

static void cpu_update_actions_state_full(double now, double delta)
{
  generic_update_actions_state_full(now, delta, surf_cpu_model);
}

xbt_dynar_t cpu_get_watts_range_list(cpu_Cas01_t cpu_model)
{
	xbt_dynar_t power_range_list = xbt_dynar_new(sizeof(xbt_dynar_t), NULL);
	xbt_dynar_t power_tuple;
	int i = 0, pstate_nb=0;
	xbt_dynar_t current_power_values;
	double min_power, max_power;
	xbt_dict_t props = cpu_model->generic_resource.properties;

	if (props == NULL)
		return NULL;

	char* all_power_values_str = xbt_dict_get_or_null(props, "power_per_state");

	if (all_power_values_str == NULL)
		return NULL;

	xbt_dynar_t all_power_values = xbt_str_split(all_power_values_str, ",");

	pstate_nb = xbt_dynar_length(all_power_values);
	for (i=0; i< pstate_nb; i++)
	{
		/* retrieve the power values associated with the current pstate */
		current_power_values = xbt_str_split(xbt_dynar_get_as(all_power_values, i, char*), ":");
		xbt_assert(xbt_dynar_length(current_power_values) > 1,
				"Power properties incorrectly defined - could not retrieve min and max power values for host %s",
				cpu_model->generic_resource.name);

		/* min_power corresponds to the idle power (cpu load = 0) */
		/* max_power is the power consumed at 100% cpu load       */
		min_power = atof(xbt_dynar_get_as(current_power_values, 0, char*));
		max_power = atof(xbt_dynar_get_as(current_power_values, 1, char*));

		power_tuple = xbt_dynar_new(sizeof(double), NULL);
		xbt_dynar_push_as(power_tuple, double, min_power);
		xbt_dynar_push_as(power_tuple, double, max_power);

		xbt_dynar_push_as(power_range_list, xbt_dynar_t, power_tuple);
	}

	return power_range_list;

}

/**
 * Computes the power consumed by the host according to the current pstate and processor load
 *
 */
static double cpu_get_current_watts_value(cpu_Cas01_t cpu_model, double cpu_load)
{
	xbt_dynar_t power_range_list = cpu_model->energy->power_range_watts_list;

	if (power_range_list == NULL)
	{
		XBT_DEBUG("No power range properties specified for host %s", cpu_model->generic_resource.name);
		return 0;
	}
	xbt_assert(xbt_dynar_length(power_range_list) == xbt_dynar_length(cpu_model->power_peak_list),
						"The number of power ranges in the properties does not match the number of pstates for host %s",
						cpu_model->generic_resource.name);

    /* retrieve the power values associated with the current pstate */
    xbt_dynar_t current_power_values = xbt_dynar_get_as(power_range_list, cpu_model->pstate, xbt_dynar_t);

    /* min_power corresponds to the idle power (cpu load = 0) */
    /* max_power is the power consumed at 100% cpu load       */
    double min_power = xbt_dynar_get_as(current_power_values, 0, double);
    double max_power = xbt_dynar_get_as(current_power_values, 1, double);
    double power_slope = max_power - min_power;

    double current_power = min_power + cpu_load * power_slope;

	XBT_DEBUG("[get_current_watts] min_power=%lf, max_power=%lf, slope=%lf", min_power, max_power, power_slope);
    XBT_DEBUG("[get_current_watts] Current power (watts) = %lf, load = %lf", current_power, cpu_load);

	return current_power;

}

/**
 * Updates the total energy consumed as the sum of the current energy and
 * 						 the energy consumed by the current action
 */
void cpu_update_energy(cpu_Cas01_t cpu_model, double cpu_load)
{

  double start_time = cpu_model->energy->last_updated;
  double finish_time = surf_get_clock();

  XBT_DEBUG("[cpu_update_energy] action time interval=(%lf-%lf), current power peak=%lf, current pstate=%d",
		  start_time, finish_time, cpu_model->power_peak, cpu_model->pstate);
  double current_energy = cpu_model->energy->total_energy;
  double action_energy = cpu_get_current_watts_value(cpu_model, cpu_load)*(finish_time-start_time);

  cpu_model->energy->total_energy = current_energy + action_energy;
  cpu_model->energy->last_updated = finish_time;

  XBT_DEBUG("[cpu_update_energy] old_energy_value=%lf, action_energy_value=%lf", current_energy, action_energy);

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
    lmm_update_constraint_bound(surf_cpu_model->model_private->maxmin_system, cpu->constraint,
                                cpu->core * cpu->power_scale *
                                cpu->power_peak);
#ifdef HAVE_TRACING
    TRACE_surf_host_set_power(date, cpu->generic_resource.name,
                              cpu->core * cpu->power_scale *
                              cpu->power_peak);
#endif
    while ((var = lmm_get_var_from_cnst
            (surf_cpu_model->model_private->maxmin_system, cpu->constraint, &elem))) {
      surf_action_cpu_Cas01_t action = lmm_variable_id(var);
      lmm_update_variable_bound(surf_cpu_model->model_private->maxmin_system,
                                GENERIC_LMM_ACTION(action).variable,
                                cpu->power_scale * cpu->power_peak);
    }
    if (tmgr_trace_event_free(event_type))
      cpu->power_event = NULL;
  } else if (event_type == cpu->state_event) {
    if (value > 0) {
      if(cpu->state_current == SURF_RESOURCE_OFF)
        xbt_dynar_push_as(host_that_restart, char*, (cpu->generic_resource.name));
      cpu->state_current = SURF_RESOURCE_ON;
    } else {
      lmm_constraint_t cnst = cpu->constraint;

      cpu->state_current = SURF_RESOURCE_OFF;

      while ((var = lmm_get_var_from_cnst(surf_cpu_model->model_private->maxmin_system, cnst, &elem))) {
        surf_action_t action = lmm_variable_id(var);

        if (surf_action_state_get(action) == SURF_ACTION_RUNNING ||
            surf_action_state_get(action) == SURF_ACTION_READY ||
            surf_action_state_get(action) ==
            SURF_ACTION_NOT_IN_THE_SYSTEM) {
          action->finish = date;
          surf_action_state_set(action, SURF_ACTION_FAILED);
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
  //xbt_dict_cursor_t cursor = NULL;
  cpu_Cas01_t CPU = surf_cpu_resource_priv(cpu);
  //xbt_dict_t props = CPU->generic_resource.properties;

  XBT_IN("(%s,%g)", surf_resource_name(CPU), size);
  action =
      surf_action_new(sizeof(s_surf_action_cpu_Cas01_t), size,
                      surf_cpu_model,
                      CPU->state_current != SURF_RESOURCE_ON);

  GENERIC_LMM_ACTION(action).suspended = 0;     /* Should be useless because of the
                                                   calloc but it seems to help valgrind... */

  GENERIC_LMM_ACTION(action).variable =
      lmm_variable_new(surf_cpu_model->model_private->maxmin_system, action,
                       GENERIC_ACTION(action).priority,
                       CPU->power_scale * CPU->power_peak, 1);
  if (surf_cpu_model->model_private->update_mechanism == UM_LAZY) {
    GENERIC_LMM_ACTION(action).index_heap = -1;
    GENERIC_LMM_ACTION(action).last_update = surf_get_clock();
    GENERIC_LMM_ACTION(action).last_value = 0.0;
  }

  lmm_expand(surf_cpu_model->model_private->maxmin_system, CPU->constraint,
             GENERIC_LMM_ACTION(action).variable, 1.0);
  XBT_OUT();
  return (surf_action_t) action;
}

static surf_action_t cpu_action_sleep(void *cpu, double duration)
{
  surf_action_cpu_Cas01_t action = NULL;

  if (duration > 0)
    duration = MAX(duration, MAXMIN_PRECISION);

  XBT_IN("(%s,%g)", surf_resource_name(surf_cpu_resource_priv(cpu)), duration);
  action = (surf_action_cpu_Cas01_t) cpu_execute(cpu, 1.0);
  // FIXME: sleep variables should not consume 1.0 in lmm_expand
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

  lmm_update_variable_weight(surf_cpu_model->model_private->maxmin_system,
                             GENERIC_LMM_ACTION(action).variable, 0.0);
  if (surf_cpu_model->model_private->update_mechanism == UM_LAZY) {     // remove action from the heap
    surf_action_lmm_heap_remove(surf_cpu_model->model_private->action_heap,(surf_action_lmm_t)action);
    // this is necessary for a variable with weight 0 since such
    // variables are ignored in lmm and we need to set its max_duration
    // correctly at the next call to share_resources
    xbt_swag_insert_at_head(action,surf_cpu_model->model_private->modified_set);
  }

  XBT_OUT();
  return (surf_action_t) action;
}

static e_surf_resource_state_t cpu_get_state(void *cpu)
{
  return ((cpu_Cas01_t)surf_cpu_resource_priv(cpu))->state_current;
}

static double cpu_get_speed(void *cpu, double load)
{
  return load * ((cpu_Cas01_t)surf_cpu_resource_priv(cpu))->power_peak;
}

static int cpu_get_core(void *cpu)
{
  return ((cpu_Cas01_t)surf_cpu_resource_priv(cpu))->core;
}


static double cpu_get_available_speed(void *cpu)
{
  /* number between 0 and 1 */
  return ((cpu_Cas01_t)surf_cpu_resource_priv(cpu))->power_scale;
}

static double cpu_get_current_power_peak(void *cpu)
{
  return ((cpu_Cas01_t)surf_cpu_resource_priv(cpu))->power_peak;
}

static double cpu_get_power_peak_at(void *cpu, int pstate_index)
{
  xbt_dynar_t plist = ((cpu_Cas01_t)surf_cpu_resource_priv(cpu))->power_peak_list;
  xbt_assert((pstate_index <= xbt_dynar_length(plist)), "Invalid parameters (pstate index out of bounds)");

  return xbt_dynar_get_as(plist, pstate_index, double);
}

static int cpu_get_nb_pstates(void *cpu)
{
  return xbt_dynar_length(((cpu_Cas01_t)surf_cpu_resource_priv(cpu))->power_peak_list);
}

static void cpu_set_power_peak_at(void *cpu, int pstate_index)
{
  cpu_Cas01_t cpu_implem = (cpu_Cas01_t)surf_cpu_resource_priv(cpu);
  xbt_dynar_t plist = cpu_implem->power_peak_list;
  xbt_assert((pstate_index <= xbt_dynar_length(plist)), "Invalid parameters (pstate index out of bounds)");

  double new_power_peak = xbt_dynar_get_as(plist, pstate_index, double);
  cpu_implem->pstate = pstate_index;
  cpu_implem->power_peak = new_power_peak;
}

static double cpu_get_consumed_energy(void *cpu)
{
  return ((cpu_Cas01_t)surf_cpu_resource_priv(cpu))->energy->total_energy;
}

static void cpu_finalize(void)
{
  lmm_system_free(surf_cpu_model->model_private->maxmin_system);
  surf_cpu_model->model_private->maxmin_system = NULL;

  if (surf_cpu_model->model_private->action_heap)
    xbt_heap_free(surf_cpu_model->model_private->action_heap);
  xbt_swag_free(surf_cpu_model->model_private->modified_set);

  surf_model_exit(surf_cpu_model);
  surf_cpu_model = NULL;

  xbt_swag_free(cpu_running_action_set_that_does_not_need_being_checked);
  cpu_running_action_set_that_does_not_need_being_checked = NULL;
}

static void surf_cpu_model_init_internal()
{
  s_surf_action_t action;
  s_surf_action_cpu_Cas01_t comp;

  char *optim = xbt_cfg_get_string(_sg_cfg_set, "cpu/optim");
  int select =
      xbt_cfg_get_boolean(_sg_cfg_set, "cpu/maxmin_selective_update");

  surf_cpu_model = surf_model_init();

  if (!strcmp(optim, "Full")) {
    surf_cpu_model->model_private->update_mechanism = UM_FULL;
    surf_cpu_model->model_private->selective_update = select;
  } else if (!strcmp(optim, "Lazy")) {
    surf_cpu_model->model_private->update_mechanism = UM_LAZY;
    surf_cpu_model->model_private->selective_update = 1;
    xbt_assert((select == 1)
               ||
               (xbt_cfg_is_default_value
                (_sg_cfg_set, "cpu/maxmin_selective_update")),
               "Disabling selective update while using the lazy update mechanism is dumb!");
  } else {
    xbt_die("Unsupported optimization (%s) for this model", optim);
  }

  cpu_running_action_set_that_does_not_need_being_checked =
      xbt_swag_new(xbt_swag_offset(action, state_hookup));

  surf_cpu_model->name = "cpu";

  surf_cpu_model->action_unref = surf_action_unref;
  surf_cpu_model->action_cancel = surf_action_cancel;
  surf_cpu_model->action_state_set = surf_action_state_set;

  surf_cpu_model->model_private->resource_used = cpu_resource_used;

  if (surf_cpu_model->model_private->update_mechanism == UM_LAZY) {
    surf_cpu_model->model_private->share_resources =
        cpu_share_resources_lazy;
    surf_cpu_model->model_private->update_actions_state =
        cpu_update_actions_state_lazy;
  } else if (surf_cpu_model->model_private->update_mechanism == UM_FULL) {
    surf_cpu_model->model_private->share_resources =
        cpu_share_resources_full;
    surf_cpu_model->model_private->update_actions_state =
        cpu_update_actions_state_full;
  } else
    xbt_die("Invalid cpu update mechanism!");

  surf_cpu_model->model_private->update_resource_state =
      cpu_update_resource_state;
  surf_cpu_model->model_private->finalize = cpu_finalize;

  surf_cpu_model->suspend = surf_action_suspend;
  surf_cpu_model->resume = surf_action_resume;
  surf_cpu_model->is_suspended = surf_action_is_suspended;
  surf_cpu_model->set_max_duration = surf_action_set_max_duration;
  surf_cpu_model->set_priority = surf_action_set_priority;
#ifdef HAVE_TRACING
  surf_cpu_model->set_category = surf_action_set_category;
#endif
  surf_cpu_model->get_remains = surf_action_get_remains;

  surf_cpu_model->extension.cpu.execute = cpu_execute;
  surf_cpu_model->extension.cpu.sleep = cpu_action_sleep;

  surf_cpu_model->extension.cpu.get_state = cpu_get_state;
  surf_cpu_model->extension.cpu.get_core = cpu_get_core;
  surf_cpu_model->extension.cpu.get_speed = cpu_get_speed;
  surf_cpu_model->extension.cpu.get_available_speed =
      cpu_get_available_speed;
  surf_cpu_model->extension.cpu.get_current_power_peak = cpu_get_current_power_peak;
  surf_cpu_model->extension.cpu.get_power_peak_at = cpu_get_power_peak_at;
  surf_cpu_model->extension.cpu.get_nb_pstates = cpu_get_nb_pstates;
  surf_cpu_model->extension.cpu.set_power_peak_at = cpu_set_power_peak_at;
  surf_cpu_model->extension.cpu.get_consumed_energy = cpu_get_consumed_energy;

  surf_cpu_model->extension.cpu.add_traces = cpu_add_traces_cpu;

  if (!surf_cpu_model->model_private->maxmin_system) {
    surf_cpu_model->model_private->maxmin_system = lmm_system_new(surf_cpu_model->model_private->selective_update);
  }
  if (surf_cpu_model->model_private->update_mechanism == UM_LAZY) {
    surf_cpu_model->model_private->action_heap = xbt_heap_new(8, NULL);
    xbt_heap_set_update_callback(surf_cpu_model->model_private->action_heap,
        surf_action_lmm_update_index_heap);
    surf_cpu_model->model_private->modified_set =
        xbt_swag_new(xbt_swag_offset(comp, generic_lmm_action.action_list_hookup));
    surf_cpu_model->model_private->maxmin_system->keep_track = surf_cpu_model->model_private->modified_set;
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
  char *optim = xbt_cfg_get_string(_sg_cfg_set, "cpu/optim");

  if (surf_cpu_model)
    return;

  if (!strcmp(optim, "TI")) {
    surf_cpu_model_init_ti();
    return;
  }

  surf_cpu_model_init_internal();
  cpu_define_callbacks();
  xbt_dynar_push(model_list, &surf_cpu_model);
}
