/* Copyright (c) 2009-2011, 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "cpu_cas01.hpp"
#include "cpu_ti.hpp"
#include "maxmin_private.hpp"
#include "simgrid/sg_config.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_cpu_cas, surf_cpu,
                                "Logging specific to the SURF CPU IMPROVED module");

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

  sg_platf_host_add_cb(cpu_parse_init);
  sg_platf_postparse_add_cb(cpu_add_traces);

  Model *model_pm = surf_cpu_model_pm;
  Model *model_vm = surf_cpu_model_vm;
  xbt_dynar_push(all_existing_models, &model_pm);
  xbt_dynar_push(all_existing_models, &model_vm);
}

CpuCas01Model::CpuCas01Model() : CpuModel()
{
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

  p_cpuRunningActionSetThatDoesNotNeedBeingChecked = new ActionList();

  if (getUpdateMechanism() == UM_LAZY) {
	shareResources = &CpuCas01Model::shareResourcesLazy;
	updateActionsState = &CpuCas01Model::updateActionsStateLazy;

  } else if (getUpdateMechanism() == UM_FULL) {
	shareResources = &CpuCas01Model::shareResourcesFull;
	updateActionsState = &CpuCas01Model::updateActionsStateFull;
  } else
    xbt_die("Invalid cpu update mechanism!");

  if (!p_maxminSystem) {
    p_maxminSystem = lmm_system_new(m_selectiveUpdate);
  }

  if (getUpdateMechanism() == UM_LAZY) {
    p_actionHeap = xbt_heap_new(8, NULL);
    xbt_heap_set_update_callback(p_actionHeap,  surf_action_lmm_update_index_heap);
    p_modifiedSet = new ActionLmmList();
    p_maxminSystem->keep_track = p_modifiedSet;
  }
}

CpuCas01Model::~CpuCas01Model()
{
  lmm_system_free(p_maxminSystem);
  p_maxminSystem = NULL;

  if (p_actionHeap)
    xbt_heap_free(p_actionHeap);
  delete p_modifiedSet;

  surf_cpu_model_pm = NULL;

  delete p_cpuRunningActionSetThatDoesNotNeedBeingChecked;
}

Cpu *CpuCas01Model::createCpu(const char *name, xbt_dynar_t power_peak,
		                  int pstate, double power_scale,
                          tmgr_trace_t power_trace, int core,
                          e_surf_resource_state_t state_initial,
                          tmgr_trace_t state_trace,
                          xbt_dict_t cpu_properties)
{
  Cpu *cpu = NULL;
  sg_host_t host = sg_host_by_name(name);
  xbt_assert(!sg_host_surfcpu(host),
             "Host '%s' declared several times in the platform file",
             name);
  xbt_assert(xbt_dynar_getfirst_as(power_peak, double) > 0.0,
      "Power has to be >0.0. Did you forget to specify the mandatory power attribute?");
  xbt_assert(core > 0, "Invalid number of cores %d. Must be larger than 0", core);

  cpu = new CpuCas01(this, name, power_peak, pstate, power_scale, power_trace, core, state_initial, state_trace, cpu_properties);
  surf_callback_emit(cpuCreatedCallbacks, cpu);
  surf_callback_emit(cpuStateChangedCallbacks, cpu, SURF_RESOURCE_ON, state_initial);
  sg_host_surfcpu_set(host, cpu);

  return cpu;
}

double CpuCas01Model::shareResourcesFull(double /*now*/)
{
  return Model::shareResourcesMaxMin(getRunningActionSet(),
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
    CpuCas01 *host = static_cast<CpuCas01*>(sg_host_surfcpu(sg_host_by_name(elm)));

    xbt_assert(host, "Host %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    host->setStateEvent(tmgr_history_add_trace(history, trace, 0.0, 0, host));
  }

  xbt_dict_foreach(trace_connect_list_power, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    CpuCas01 *host = static_cast<CpuCas01*>(sg_host_surfcpu(sg_host_by_name(elm)));

    xbt_assert(host, "Host %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    host->setPowerEvent(tmgr_history_add_trace(history, trace, 0.0, 0, host));
  }
}

/************
 * Resource *
 ************/
CpuCas01::CpuCas01(CpuCas01Model *model, const char *name, xbt_dynar_t powerPeak,
                         int pstate, double powerScale, tmgr_trace_t powerTrace, int core,
                         e_surf_resource_state_t stateInitial, tmgr_trace_t stateTrace,
                         xbt_dict_t properties)
: Cpu(model, name, properties,
	  lmm_constraint_new(model->getMaxminSystem(), this, core * powerScale * xbt_dynar_get_as(powerPeak, pstate, double)),
	  core, xbt_dynar_get_as(powerPeak, pstate, double), powerScale,
    stateInitial) {
  p_powerEvent = NULL;
  p_powerPeakList = powerPeak;
  m_pstate = pstate;

  XBT_DEBUG("CPU create: peak=%f, pstate=%d", m_powerPeak, m_pstate);

  m_core = core;
  if (powerTrace)
    p_powerEvent = tmgr_history_add_trace(history, powerTrace, 0.0, 0, this);

  if (stateTrace)
    p_stateEvent = tmgr_history_add_trace(history, stateTrace, 0.0, 0, this);
}

CpuCas01::~CpuCas01(){
  if (getModel() == surf_cpu_model_pm)
    xbt_dynar_free(&p_powerPeakList);
}

void CpuCas01::setStateEvent(tmgr_trace_event_t stateEvent)
{
  p_stateEvent = stateEvent;
}

void CpuCas01::setPowerEvent(tmgr_trace_event_t powerEvent)
{
  p_powerEvent = powerEvent;
}

xbt_dynar_t CpuCas01::getPowerPeakList(){
  return p_powerPeakList;
}

int CpuCas01::getPState()
{
  return m_pstate;
}

bool CpuCas01::isUsed()
{
  return lmm_constraint_used(getModel()->getMaxminSystem(), getConstraint());
}

void CpuCas01::updateState(tmgr_trace_event_t event_type, double value, double date)
{
  lmm_variable_t var = NULL;
  lmm_element_t elem = NULL;

  if (event_type == p_powerEvent) {
	/* TODO (Hypervisor): do the same thing for constraint_core[i] */
	xbt_assert(m_core == 1, "FIXME: add power scaling code also for constraint_core[i]");

    m_powerScale = value;
    lmm_update_constraint_bound(getModel()->getMaxminSystem(), getConstraint(),
                                m_core * m_powerScale *
                                m_powerPeak);
    TRACE_surf_host_set_power(date, getName(),
                              m_core * m_powerScale *
                              m_powerPeak);
    while ((var = lmm_get_var_from_cnst
            (getModel()->getMaxminSystem(), getConstraint(), &elem))) {
      CpuCas01Action *action = static_cast<CpuCas01Action*>(lmm_variable_id(var));

      lmm_update_variable_bound(getModel()->getMaxminSystem(),
                                action->getVariable(),
                                m_powerScale * m_powerPeak);
    }
    if (tmgr_trace_event_free(event_type))
      p_powerEvent = NULL;
  } else if (event_type == p_stateEvent) {
	/* TODO (Hypervisor): do the same thing for constraint_core[i] */
    xbt_assert(m_core == 1, "FIXME: add state change code also for constraint_core[i]");

    if (value > 0) {
      if(getState() == SURF_RESOURCE_OFF)
        xbt_dynar_push_as(host_that_restart, char*, (char *)getName());
      setState(SURF_RESOURCE_ON);
    } else {
      lmm_constraint_t cnst = getConstraint();

      setState(SURF_RESOURCE_OFF);

      while ((var = lmm_get_var_from_cnst(getModel()->getMaxminSystem(), cnst, &elem))) {
        Action *action = static_cast<Action*>(lmm_variable_id(var));

        if (action->getState() == SURF_ACTION_RUNNING ||
            action->getState() == SURF_ACTION_READY ||
            action->getState() == SURF_ACTION_NOT_IN_THE_SYSTEM) {
          action->setFinishTime(date);
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

CpuAction *CpuCas01::execute(double size)
{

  XBT_IN("(%s,%g)", getName(), size);
  CpuCas01Action *action = new CpuCas01Action(getModel(), size, getState() != SURF_RESOURCE_ON,
		                                              m_powerScale * m_powerPeak, getConstraint());

  XBT_OUT();
  return action;
}

CpuAction *CpuCas01::sleep(double duration)
{
  if (duration > 0)
    duration = MAX(duration, sg_surf_precision);

  XBT_IN("(%s,%g)", getName(), duration);
  CpuCas01Action *action = new CpuCas01Action(getModel(), 1.0, getState() != SURF_RESOURCE_ON,
                                                      m_powerScale * m_powerPeak, getConstraint());


  // FIXME: sleep variables should not consume 1.0 in lmm_expand
  action->m_maxDuration = duration;
  action->m_suspended = 2;
  if (duration == NO_MAX_DURATION) {
    /* Move to the *end* of the corresponding action set. This convention
       is used to speed up update_resource_state  */
	action->getStateSet()->erase(action->getStateSet()->iterator_to(*action));
    action->p_stateSet = static_cast<CpuCas01Model*>(getModel())->p_cpuRunningActionSetThatDoesNotNeedBeingChecked;
    action->getStateSet()->push_back(*action);
  }

  lmm_update_variable_weight(getModel()->getMaxminSystem(),
                             action->getVariable(), 0.0);
  if (getModel()->getUpdateMechanism() == UM_LAZY) {     // remove action from the heap
    action->heapRemove(getModel()->getActionHeap());
    // this is necessary for a variable with weight 0 since such
    // variables are ignored in lmm and we need to set its max_duration
    // correctly at the next call to share_resources
    getModel()->getModifiedSet()->push_front(*action);
  }

  XBT_OUT();
  return action;
}

double CpuCas01::getCurrentPowerPeak()
{
  return m_powerPeak;
}

double CpuCas01::getPowerPeakAt(int pstate_index)
{
  xbt_dynar_t plist = p_powerPeakList;
  xbt_assert((pstate_index <= (int)xbt_dynar_length(plist)), "Invalid parameters (pstate index out of bounds)");

  return xbt_dynar_get_as(plist, pstate_index, double);
}

int CpuCas01::getNbPstates()
{
  return xbt_dynar_length(p_powerPeakList);
}

void CpuCas01::setPstate(int pstate_index)
{
  xbt_dynar_t plist = p_powerPeakList;
  xbt_assert((pstate_index <= (int)xbt_dynar_length(plist)), "Invalid parameters (pstate index out of bounds)");

  double new_pstate = xbt_dynar_get_as(plist, pstate_index, double);
  m_pstate = pstate_index;
  m_powerPeak = new_pstate;
}

int CpuCas01::getPstate()
{
  return m_pstate;
}

/**********
 * Action *
 **********/

CpuCas01Action::CpuCas01Action(Model *model, double cost, bool failed, double power, lmm_constraint_t constraint)
 : CpuAction(model, cost, failed,
		     lmm_variable_new(model->getMaxminSystem(), this,
		     1.0, power, 1))
{
  m_suspended = 0;
  if (model->getUpdateMechanism() == UM_LAZY) {
    m_indexHeap = -1;
    m_lastUpdate = surf_get_clock();
    m_lastValue = 0.0;
  }
  lmm_expand(model->getMaxminSystem(), constraint, getVariable(), 1.0);
}
