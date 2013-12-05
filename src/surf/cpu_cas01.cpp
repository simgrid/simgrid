/* Copyright (c) 2009-2011, 2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "cpu_cas01.hpp"
#include "cpu_ti.hpp"
#include "maxmin_private.h"
#include "simgrid/sg_config.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_cpu_cas, surf_cpu,
                                "Logging specific to the SURF CPU IMPROVED module");

/*************
 * CallBacks *
 *************/

static void parse_cpu_init(sg_platf_host_cbarg_t host){
  ((CpuCas01ModelPtr)surf_cpu_model_pm)->parseInit(host);
}

static void cpu_add_traces_cpu(){
  surf_cpu_model_pm->addTraces();
}

static void cpu_define_callbacks()
{
  sg_platf_host_add_cb(parse_cpu_init);
  sg_platf_postparse_add_cb(cpu_add_traces_cpu);
}

/*********
 * Model *
 *********/
void surf_cpu_model_init_Cas01()
{
  char *optim = xbt_cfg_get_string(_sg_cfg_set, "cpu/optim");

  xbt_assert(!surf_cpu_model_pm);
  xbt_assert(!surf_cpu_model_vm);

  if (!strcmp(optim, "TI")) {
    surf_cpu_model_init_ti();
    return;
  }

  surf_cpu_model_pm = new CpuCas01Model();
  surf_cpu_model_vm  = new CpuCas01Model();

  cpu_define_callbacks();
  ModelPtr model_pm = static_cast<ModelPtr>(surf_cpu_model_pm);
  ModelPtr model_vm = static_cast<ModelPtr>(surf_cpu_model_vm);
  xbt_dynar_push(model_list, &model_pm);
  xbt_dynar_push(model_list, &model_vm);
}

CpuCas01Model::CpuCas01Model() : CpuModel("cpu")
{
  ActionPtr action = NULL;
  ActionLmmPtr actionlmm = NULL;

  char *optim = xbt_cfg_get_string(_sg_cfg_set, "cpu/optim");
  int select = xbt_cfg_get_boolean(_sg_cfg_set, "cpu/maxmin_selective_update");

  if (!strcmp(optim, "Full")) {
    p_updateMechanism = UM_FULL;
    m_selectiveUpdate = select;
  } else if (!strcmp(optim, "Lazy")) {
    p_updateMechanism = UM_LAZY;
    m_selectiveUpdate = 1;
    xbt_assert((select == 1)
               ||
               (xbt_cfg_is_default_value
                (_sg_cfg_set, "cpu/maxmin_selective_update")),
               "Disabling selective update while using the lazy update mechanism is dumb!");
  } else {
    xbt_die("Unsupported optimization (%s) for this model", optim);
  }

  p_cpuRunningActionSetThatDoesNotNeedBeingChecked =
      xbt_swag_new(xbt_swag_offset(*action, p_stateHookup));

  if (p_updateMechanism == UM_LAZY) {
	shareResources = &CpuCas01Model::shareResourcesLazy;
	updateActionsState = &CpuCas01Model::updateActionsStateLazy;

  } else if (p_updateMechanism == UM_FULL) {
	shareResources = &CpuCas01Model::shareResourcesFull;
	updateActionsState = &CpuCas01Model::updateActionsStateFull;
  } else
    xbt_die("Invalid cpu update mechanism!");

  if (!p_maxminSystem) {
    p_maxminSystem = lmm_system_new(m_selectiveUpdate);
  }

  if (p_updateMechanism == UM_LAZY) {
    p_actionHeap = xbt_heap_new(8, NULL);
    xbt_heap_set_update_callback(p_actionHeap,  surf_action_lmm_update_index_heap);
    p_modifiedSet = xbt_swag_new(xbt_swag_offset(*actionlmm, p_actionListHookup));
    p_maxminSystem->keep_track = p_modifiedSet;
  }
}

CpuCas01Model::~CpuCas01Model()
{
  lmm_system_free(p_maxminSystem);
  p_maxminSystem = NULL;

  if (p_actionHeap)
    xbt_heap_free(p_actionHeap);
  xbt_swag_free(p_modifiedSet);

  surf_cpu_model_pm = NULL;

  xbt_swag_free(p_cpuRunningActionSetThatDoesNotNeedBeingChecked);
}

void CpuCas01Model::parseInit(sg_platf_host_cbarg_t host)
{
  createResource(host->id,
        host->power_peak,
        host->pstate,
        host->power_scale,
        host->power_trace,
        host->core_amount,
        host->initial_state,
        host->state_trace,
        host->properties);
}

CpuPtr CpuCas01Model::createResource(const char *name, xbt_dynar_t power_peak,
		                  int pstate, double power_scale,
                          tmgr_trace_t power_trace, int core,
                          e_surf_resource_state_t state_initial,
                          tmgr_trace_t state_trace,
                          xbt_dict_t cpu_properties)
{
  CpuPtr cpu = NULL;
  xbt_assert(!surf_cpu_resource_priv(surf_cpu_resource_by_name(name)),
             "Host '%s' declared several times in the platform file",
             name);
  xbt_assert(xbt_dynar_getfirst_as(power_peak, double) > 0.0,
      "Power has to be >0.0");
  xbt_assert(core > 0, "Invalid number of cores %d", core);

  cpu = new CpuCas01Lmm(this, name, power_peak, pstate, power_scale, power_trace, core, state_initial, state_trace, cpu_properties);
  xbt_lib_set(host_lib, name, SURF_CPU_LEVEL, static_cast<ResourcePtr>(cpu));

  return cpu;
}

double CpuCas01Model::shareResourcesFull(double /*now*/)
{
  return Model::shareResourcesMaxMin(p_runningActionSet,
                             p_maxminSystem, lmm_solve);
}

void CpuCas01Model::addTraces()
{
  xbt_dict_cursor_t cursor = NULL;
  char *trace_name, *elm;
  static int called = 0;
  if (called)
    return;
  called = 1;

  /* connect all traces relative to hosts */
  xbt_dict_foreach(trace_connect_list_host_avail, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    CpuCas01LmmPtr host = static_cast<CpuCas01LmmPtr>(surf_cpu_resource_priv(surf_cpu_resource_by_name(elm)));

    xbt_assert(host, "Host %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    host->p_stateEvent =
        tmgr_history_add_trace(history, trace, 0.0, 0, static_cast<ResourcePtr>(host));
  }

  xbt_dict_foreach(trace_connect_list_power, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    CpuCas01LmmPtr host = dynamic_cast<CpuCas01LmmPtr>(static_cast<ResourcePtr>(surf_cpu_resource_priv(surf_cpu_resource_by_name(elm))));

    xbt_assert(host, "Host %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    host->p_powerEvent =
        tmgr_history_add_trace(history, trace, 0.0, 0, static_cast<ResourcePtr>(host));
  }
}

/************
 * Resource *
 ************/
CpuCas01Lmm::CpuCas01Lmm(CpuCas01ModelPtr model, const char *name, xbt_dynar_t powerPeak,
                         int pstate, double powerScale, tmgr_trace_t powerTrace, int core,
                         e_surf_resource_state_t stateInitial, tmgr_trace_t stateTrace,
                         xbt_dict_t properties)
: Resource(model, name, properties)
, CpuLmm(model, name, properties, core, xbt_dynar_get_as(powerPeak, pstate, double), powerScale) {
  p_powerEvent = NULL;
  p_powerPeakList = powerPeak;
  m_pstate = pstate;

  p_energy = xbt_new(s_energy_cpu_cas01_t, 1);
  p_energy->total_energy = 0;
  p_energy->power_range_watts_list = getWattsRangeList();
  p_energy->last_updated = surf_get_clock();

  XBT_DEBUG("CPU create: peak=%f, pstate=%d", m_powerPeak, m_pstate);

  m_core = core;
  p_stateCurrent = stateInitial;
  if (powerTrace)
    p_powerEvent = tmgr_history_add_trace(history, powerTrace, 0.0, 0, static_cast<ResourcePtr>(this));

  if (stateTrace)
    p_stateEvent = tmgr_history_add_trace(history, stateTrace, 0.0, 0, static_cast<ResourcePtr>(this));

  p_constraint = lmm_constraint_new(p_model->p_maxminSystem, this, m_core * m_powerScale * m_powerPeak);
}

CpuCas01Lmm::~CpuCas01Lmm(){
  unsigned int iter;
  xbt_dynar_t power_tuple = NULL;
  xbt_dynar_foreach(p_energy->power_range_watts_list, iter, power_tuple)
    xbt_dynar_free(&power_tuple);
  xbt_dynar_free(&p_energy->power_range_watts_list);
  xbt_dynar_free(&p_powerPeakList);
  xbt_free(p_energy);
}

bool CpuCas01Lmm::isUsed()
{
  return lmm_constraint_used(p_model->p_maxminSystem, p_constraint);
}

void CpuCas01Lmm::updateState(tmgr_trace_event_t event_type, double value, double date)
{
  lmm_variable_t var = NULL;
  lmm_element_t elem = NULL;

  if (event_type == p_powerEvent) {
	/* TODO (Hypervisor): do the same thing for constraint_core[i] */
	xbt_assert(m_core == 1, "FIXME: add power scaling code also for constraint_core[i]");

    m_powerScale = value;
    lmm_update_constraint_bound(surf_cpu_model_pm->p_maxminSystem, p_constraint,
                                m_core * m_powerScale *
                                m_powerPeak);
#ifdef HAVE_TRACING
    TRACE_surf_host_set_power(date, m_name,
                              m_core * m_powerScale *
                              m_powerPeak);
#endif
    while ((var = lmm_get_var_from_cnst
            (surf_cpu_model_pm->p_maxminSystem, p_constraint, &elem))) {
      CpuCas01ActionLmmPtr action = static_cast<CpuCas01ActionLmmPtr>(static_cast<ActionLmmPtr>(lmm_variable_id(var)));

      lmm_update_variable_bound(surf_cpu_model_pm->p_maxminSystem,
                                action->p_variable,
                                m_powerScale * m_powerPeak);
    }
    if (tmgr_trace_event_free(event_type))
      p_powerEvent = NULL;
  } else if (event_type == p_stateEvent) {
	/* TODO (Hypervisor): do the same thing for constraint_core[i] */
    xbt_assert(m_core == 1, "FIXME: add state change code also for constraint_core[i]");

    if (value > 0) {
      if(p_stateCurrent == SURF_RESOURCE_OFF)
        xbt_dynar_push_as(host_that_restart, char*, (char *)m_name);
      p_stateCurrent = SURF_RESOURCE_ON;
    } else {
      lmm_constraint_t cnst = p_constraint;

      p_stateCurrent = SURF_RESOURCE_OFF;

      while ((var = lmm_get_var_from_cnst(surf_cpu_model_pm->p_maxminSystem, cnst, &elem))) {
        ActionLmmPtr action = static_cast<ActionLmmPtr>(lmm_variable_id(var));

        if (action->getState() == SURF_ACTION_RUNNING ||
            action->getState() == SURF_ACTION_READY ||
            action->getState() == SURF_ACTION_NOT_IN_THE_SYSTEM) {
          action->m_finish = date;
          action->setState(SURF_ACTION_FAILED);
        }
      }
    }
    if (tmgr_trace_event_free(event_type))
      p_stateEvent = NULL;
  } else {
    XBT_CRITICAL("Unknown event ! \n");
    xbt_abort();
  }

  return;
}

ActionPtr CpuCas01Lmm::execute(double size)
{

  XBT_IN("(%s,%g)", m_name, size);
  CpuCas01ActionLmmPtr action = new CpuCas01ActionLmm(surf_cpu_model_pm, size, p_stateCurrent != SURF_RESOURCE_ON);

  action->m_suspended = 0;     /* Should be useless because of the
                                                   calloc but it seems to help valgrind... */

  action->p_variable =
      lmm_variable_new(surf_cpu_model_pm->p_maxminSystem, static_cast<ActionLmmPtr>(action),
                       action->m_priority,
                       m_powerScale * m_powerPeak, 1);
  if (surf_cpu_model_pm->p_updateMechanism == UM_LAZY) {
    action->m_indexHeap = -1;
    action->m_lastUpdate = surf_get_clock();
    action->m_lastValue = 0.0;
  }
  lmm_expand(surf_cpu_model_pm->p_maxminSystem, p_constraint,
             action->p_variable, 1.0);
  XBT_OUT();
  return action;
}

ActionPtr CpuCas01Lmm::sleep(double duration)
{
  if (duration > 0)
    duration = MAX(duration, MAXMIN_PRECISION);

  XBT_IN("(%s,%g)", m_name, duration);
  CpuCas01ActionLmmPtr action = dynamic_cast<CpuCas01ActionLmmPtr>(execute(1.0));

  // FIXME: sleep variables should not consume 1.0 in lmm_expand
  action->m_maxDuration = duration;
  action->m_suspended = 2;
  if (duration == NO_MAX_DURATION) {
    /* Move to the *end* of the corresponding action set. This convention
       is used to speed up update_resource_state  */
    xbt_swag_remove(static_cast<ActionPtr>(action), action->p_stateSet);
    action->p_stateSet = static_cast<CpuCas01ModelPtr>(p_model)->p_cpuRunningActionSetThatDoesNotNeedBeingChecked;
    xbt_swag_insert(static_cast<ActionPtr>(action), action->p_stateSet);
  }

  lmm_update_variable_weight(surf_cpu_model_pm->p_maxminSystem,
                             action->p_variable, 0.0);
  if (surf_cpu_model_pm->p_updateMechanism == UM_LAZY) {     // remove action from the heap
    action->heapRemove(surf_cpu_model_pm->p_actionHeap);
    // this is necessary for a variable with weight 0 since such
    // variables are ignored in lmm and we need to set its max_duration
    // correctly at the next call to share_resources
    xbt_swag_insert_at_head(static_cast<ActionLmmPtr>(action), surf_cpu_model_pm->p_modifiedSet);
  }

  XBT_OUT();
  return action;
}

xbt_dynar_t CpuCas01Lmm::getWattsRangeList()
{
	xbt_dynar_t power_range_list;
	xbt_dynar_t power_tuple;
	int i = 0, pstate_nb=0;
	xbt_dynar_t current_power_values;
	double min_power, max_power;

	if (m_properties == NULL)
		return NULL;

	char* all_power_values_str = (char*)xbt_dict_get_or_null(m_properties, "power_per_state");

	if (all_power_values_str == NULL)
		return NULL;


	power_range_list = xbt_dynar_new(sizeof(xbt_dynar_t), NULL);
	xbt_dynar_t all_power_values = xbt_str_split(all_power_values_str, ",");

	pstate_nb = xbt_dynar_length(all_power_values);
	for (i=0; i< pstate_nb; i++)
	{
		/* retrieve the power values associated with the current pstate */
		current_power_values = xbt_str_split(xbt_dynar_get_as(all_power_values, i, char*), ":");
		xbt_assert(xbt_dynar_length(current_power_values) > 1,
				"Power properties incorrectly defined - could not retrieve min and max power values for host %s",
				m_name);

		/* min_power corresponds to the idle power (cpu load = 0) */
		/* max_power is the power consumed at 100% cpu load       */
		min_power = atof(xbt_dynar_get_as(current_power_values, 0, char*));
		max_power = atof(xbt_dynar_get_as(current_power_values, 1, char*));

		power_tuple = xbt_dynar_new(sizeof(double), NULL);
		xbt_dynar_push_as(power_tuple, double, min_power);
		xbt_dynar_push_as(power_tuple, double, max_power);

		xbt_dynar_push_as(power_range_list, xbt_dynar_t, power_tuple);
		xbt_dynar_free(&current_power_values);
	}
	xbt_dynar_free(&all_power_values);
	return power_range_list;
}

/**
 * Computes the power consumed by the host according to the current pstate and processor load
 *
 */
double CpuCas01Lmm::getCurrentWattsValue(double cpu_load)
{
	xbt_dynar_t power_range_list = p_energy->power_range_watts_list;

	if (power_range_list == NULL)
	{
		XBT_DEBUG("No power range properties specified for host %s", m_name);
		return 0;
	}
	xbt_assert(xbt_dynar_length(power_range_list) == xbt_dynar_length(p_powerPeakList),
						"The number of power ranges in the properties does not match the number of pstates for host %s",
						m_name);

    /* retrieve the power values associated with the current pstate */
    xbt_dynar_t current_power_values = xbt_dynar_get_as(power_range_list, m_pstate, xbt_dynar_t);

    /* min_power corresponds to the idle power (cpu load = 0) */
    /* max_power is the power consumed at 100% cpu load       */
    double min_power = xbt_dynar_get_as(current_power_values, 0, double);
    double max_power = xbt_dynar_get_as(current_power_values, 1, double);
    double power_slope = max_power - min_power;

    double current_power = min_power + cpu_load * power_slope;

	XBT_DEBUG("[get_current_watts] min_power=%f, max_power=%f, slope=%f", min_power, max_power, power_slope);
    XBT_DEBUG("[get_current_watts] Current power (watts) = %f, load = %f", current_power, cpu_load);

	return current_power;
}

/**
 * Updates the total energy consumed as the sum of the current energy and
 * 						 the energy consumed by the current action
 */
void CpuCas01Lmm::updateEnergy(double cpu_load)
{
  double start_time = p_energy->last_updated;
  double finish_time = surf_get_clock();

  XBT_DEBUG("[cpu_update_energy] action time interval=(%f-%f), current power peak=%f, current pstate=%d",
		  start_time, finish_time, m_powerPeak, m_pstate);
  double current_energy = p_energy->total_energy;
  double action_energy = getCurrentWattsValue(cpu_load)*(finish_time-start_time);

  p_energy->total_energy = current_energy + action_energy;
  p_energy->last_updated = finish_time;

  XBT_DEBUG("[cpu_update_energy] old_energy_value=%f, action_energy_value=%f", current_energy, action_energy);
}

double CpuCas01Lmm::getCurrentPowerPeak()
{
  return m_powerPeak;
}

double CpuCas01Lmm::getPowerPeakAt(int pstate_index)
{
  xbt_dynar_t plist = p_powerPeakList;
  xbt_assert((pstate_index <= (int)xbt_dynar_length(plist)), "Invalid parameters (pstate index out of bounds)");

  return xbt_dynar_get_as(plist, pstate_index, double);
}

int CpuCas01Lmm::getNbPstates()
{
  return xbt_dynar_length(p_powerPeakList);
}

void CpuCas01Lmm::setPowerPeakAt(int pstate_index)
{
  xbt_dynar_t plist = p_powerPeakList;
  xbt_assert((pstate_index <= (int)xbt_dynar_length(plist)), "Invalid parameters (pstate index out of bounds)");

  double new_power_peak = xbt_dynar_get_as(plist, pstate_index, double);
  m_pstate = pstate_index;
  m_powerPeak = new_power_peak;
}

double CpuCas01Lmm::getConsumedEnergy()
{
  return p_energy->total_energy;
}

/**********
 * Action *
 **********/

/**
 * Update the CPU total energy for a finished action
 *
 */
void CpuCas01ActionLmm::updateEnergy()
{
  CpuCas01LmmPtr cpu  = static_cast<CpuCas01LmmPtr>(lmm_constraint_id(lmm_get_cnst_from_var
                	  	  	  	  	  	  	  	  (p_model->p_maxminSystem,
                	  	  	  	  	  	  	  			  p_variable, 0)));

  if(cpu->p_energy->last_updated < surf_get_clock()) {
   	double load = lmm_constraint_get_usage(cpu->p_constraint) / cpu->m_powerPeak;
    cpu->updateEnergy(load);
  }
}
