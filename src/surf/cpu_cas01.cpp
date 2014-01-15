/* Copyright (c) 2009-2011, 2013. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "cpu_cas01.hpp"
#include "cpu_ti.hpp"
#include "plugins/energy.hpp"
#include "maxmin_private.hpp"
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

  cpu = new CpuCas01(this, name, power_peak, pstate, power_scale, power_trace, core, state_initial, state_trace, cpu_properties);
  xbt_lib_set(host_lib, name, SURF_CPU_LEVEL, static_cast<ResourcePtr>(cpu));

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
    CpuCas01Ptr host = static_cast<CpuCas01Ptr>(surf_cpu_resource_priv(surf_cpu_resource_by_name(elm)));

    xbt_assert(host, "Host %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    host->p_stateEvent = tmgr_history_add_trace(history, trace, 0.0, 0, static_cast<ResourcePtr>(host));
  }

  xbt_dict_foreach(trace_connect_list_power, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    CpuCas01Ptr host = static_cast<CpuCas01Ptr>(surf_cpu_resource_priv(surf_cpu_resource_by_name(elm)));

    xbt_assert(host, "Host %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    host->p_powerEvent =
        tmgr_history_add_trace(history, trace, 0.0, 0, static_cast<ResourcePtr>(host));
  }
}

/************
 * Resource *
 ************/
CpuCas01::CpuCas01(CpuCas01ModelPtr model, const char *name, xbt_dynar_t powerPeak,
                         int pstate, double powerScale, tmgr_trace_t powerTrace, int core,
                         e_surf_resource_state_t stateInitial, tmgr_trace_t stateTrace,
                         xbt_dict_t properties)
: Cpu(model, name, properties,
	  lmm_constraint_new(model->getMaxminSystem(), this, core * powerScale * xbt_dynar_get_as(powerPeak, pstate, double)),
	  core, xbt_dynar_get_as(powerPeak, pstate, double), powerScale) {
  p_powerEvent = NULL;
  p_powerPeakList = powerPeak;
  m_pstate = pstate;

  XBT_DEBUG("CPU create: peak=%f, pstate=%d", m_powerPeak, m_pstate);

  m_core = core;
  m_stateCurrent = stateInitial;
  if (powerTrace)
    p_powerEvent = tmgr_history_add_trace(history, powerTrace, 0.0, 0, static_cast<ResourcePtr>(this));

  if (stateTrace)
    p_stateEvent = tmgr_history_add_trace(history, stateTrace, 0.0, 0, static_cast<ResourcePtr>(this));
}

CpuCas01::~CpuCas01(){
  xbt_dynar_free(&p_powerPeakList);
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
    lmm_update_constraint_bound(surf_cpu_model_pm->getMaxminSystem(), getConstraint(),
                                m_core * m_powerScale *
                                m_powerPeak);
#ifdef HAVE_TRACING
    TRACE_surf_host_set_power(date, getName(),
                              m_core * m_powerScale *
                              m_powerPeak);
#endif
    while ((var = lmm_get_var_from_cnst
            (surf_cpu_model_pm->getMaxminSystem(), getConstraint(), &elem))) {
      CpuCas01ActionPtr action = static_cast<CpuCas01ActionPtr>(static_cast<ActionPtr>(lmm_variable_id(var)));

      lmm_update_variable_bound(surf_cpu_model_pm->getMaxminSystem(),
                                action->getVariable(),
                                m_powerScale * m_powerPeak);
    }
    if (tmgr_trace_event_free(event_type))
      p_powerEvent = NULL;
  } else if (event_type == p_stateEvent) {
	/* TODO (Hypervisor): do the same thing for constraint_core[i] */
    xbt_assert(m_core == 1, "FIXME: add state change code also for constraint_core[i]");

    if (value > 0) {
      if(m_stateCurrent == SURF_RESOURCE_OFF)
        xbt_dynar_push_as(host_that_restart, char*, (char *)getName());
      m_stateCurrent = SURF_RESOURCE_ON;
    } else {
      lmm_constraint_t cnst = getConstraint();

      m_stateCurrent = SURF_RESOURCE_OFF;

      while ((var = lmm_get_var_from_cnst(surf_cpu_model_pm->getMaxminSystem(), cnst, &elem))) {
        ActionPtr action = static_cast<ActionPtr>(lmm_variable_id(var));

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

CpuActionPtr CpuCas01::execute(double size)
{

  XBT_IN("(%s,%g)", getName(), size);
  CpuCas01ActionPtr action = new CpuCas01Action(surf_cpu_model_pm, size, m_stateCurrent != SURF_RESOURCE_ON,
		                                              m_powerScale * m_powerPeak, getConstraint());

  XBT_OUT();
  return action;
}

CpuActionPtr CpuCas01::sleep(double duration)
{
  if (duration > 0)
    duration = MAX(duration, MAXMIN_PRECISION);

  XBT_IN("(%s,%g)", getName(), duration);
  CpuCas01ActionPtr action = new CpuCas01Action(surf_cpu_model_pm, 1.0, m_stateCurrent != SURF_RESOURCE_ON,
                                                      m_powerScale * m_powerPeak, getConstraint());


  // FIXME: sleep variables should not consume 1.0 in lmm_expand
  action->m_maxDuration = duration;
  action->m_suspended = 2;
  if (duration == NO_MAX_DURATION) {
    /* Move to the *end* of the corresponding action set. This convention
       is used to speed up update_resource_state  */
	action->getStateSet()->erase(action->getStateSet()->iterator_to(*action));
    action->p_stateSet = static_cast<CpuCas01ModelPtr>(getModel())->p_cpuRunningActionSetThatDoesNotNeedBeingChecked;
    action->getStateSet()->push_back(*action);
  }

  lmm_update_variable_weight(surf_cpu_model_pm->getMaxminSystem(),
                             action->getVariable(), 0.0);
  if (surf_cpu_model_pm->getUpdateMechanism() == UM_LAZY) {     // remove action from the heap
    action->heapRemove(surf_cpu_model_pm->getActionHeap());
    // this is necessary for a variable with weight 0 since such
    // variables are ignored in lmm and we need to set its max_duration
    // correctly at the next call to share_resources
    surf_cpu_model_pm->getModifiedSet()->push_front(*action);
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

void CpuCas01::setPowerPeakAt(int pstate_index)
{
  xbt_dynar_t plist = p_powerPeakList;
  xbt_assert((pstate_index <= (int)xbt_dynar_length(plist)), "Invalid parameters (pstate index out of bounds)");

  double new_power_peak = xbt_dynar_get_as(plist, pstate_index, double);
  m_pstate = pstate_index;
  m_powerPeak = new_power_peak;
}

/**********
 * Action *
 **********/

CpuCas01Action::CpuCas01Action(ModelPtr model, double cost, bool failed, double power, lmm_constraint_t constraint)
 : CpuAction(model, cost, failed,
		     lmm_variable_new(model->getMaxminSystem(), static_cast<ActionPtr>(this),
		     1.0, power, 1))
{
  m_suspended = 0;     /* Should be useless because of the
	                                                   calloc but it seems to help valgrind... */

  if (model->getUpdateMechanism() == UM_LAZY) {
    m_indexHeap = -1;
    m_lastUpdate = surf_get_clock();
    m_lastValue = 0.0;
  }
  lmm_expand(model->getMaxminSystem(), constraint, getVariable(), 1.0);
}
