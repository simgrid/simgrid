/* Copyright (c) 2009-2011. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "cpu_cas01.hpp"
#include "cpu_ti.hpp"
#include "surf.hpp"
#include "maxmin_private.h"
#include "simgrid/sg_config.h"

extern "C" {
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_cpu_cas, surf,
                                "Logging specific to the SURF CPU IMPROVED module");
}

static xbt_swag_t
    cpu_running_action_set_that_does_not_need_being_checked = NULL;

/*************
 * CallBacks *
 *************/

static void parse_cpu_init(sg_platf_host_cbarg_t host){
  ((CpuCas01ModelPtr)surf_cpu_model)->parseInit(host);
}

static void cpu_add_traces_cpu(){
  surf_cpu_model->addTraces();
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

  if (surf_cpu_model)
    return;

  if (!strcmp(optim, "TI")) {
    surf_cpu_model = new CpuTiModel();
    return;
  }

  surf_cpu_model = new CpuCas01Model();
  cpu_define_callbacks();
  ModelPtr model = static_cast<ModelPtr>(surf_cpu_model);
  xbt_dynar_push(model_list, &model);
}

CpuCas01Model::CpuCas01Model() : CpuModel("cpu")
{
  CpuCas01ActionLmm action;

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

  cpu_running_action_set_that_does_not_need_being_checked =
      xbt_swag_new(xbt_swag_offset(action, p_stateHookup));

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
    ActionLmmPtr _actionlmm;
    CpuCas01ActionLmmPtr _actioncpu;
    int j = xbt_swag_offset(*_actionlmm, p_actionListHookup);
    int k = xbt_swag_offset(*_actioncpu, p_actionListHookup);
    j = ((char *)&( (*_actionlmm).p_actionListHookup ) - (char *)(_actionlmm));
    k = ((char *)&( (*_actioncpu).p_actionListHookup ) - (char *)(_actioncpu));
    void *toto = &(*_actionlmm).p_actionListHookup;
    void *tata = _actionlmm;
    ActionLmm aieu;
    ActionLmmPtr actionBase = &aieu;
    void *actionBaseVoid = actionBase;
    void *actionBaseCVoid = static_cast<void*>(actionBase);
    ActionLmmPtr actionBaseVoidBase = (ActionLmmPtr)actionBaseVoid;
    ActionLmmPtr actionBaseCVoidCBase = static_cast<ActionLmmPtr>(actionBaseCVoid);
    p_modifiedSet = xbt_swag_new(xbt_swag_offset(*_actionlmm, p_actionListHookup));
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

  surf_cpu_model = NULL;

  xbt_swag_free(cpu_running_action_set_that_does_not_need_being_checked);
  cpu_running_action_set_that_does_not_need_being_checked = NULL;
}

void CpuCas01Model::parseInit(sg_platf_host_cbarg_t host)
{
  createResource(host->id,
        host->power_peak,
        host->power_scale,
        host->power_trace,
        host->core_amount,
        host->initial_state,
        host->state_trace,
        host->properties);
}
CpuCas01LmmPtr CpuCas01Model::createResource(const char *name, double power_peak, double power_scale,
                          tmgr_trace_t power_trace, int core,
                          e_surf_resource_state_t state_initial,
                          tmgr_trace_t state_trace,
                          xbt_dict_t cpu_properties)
{
  CpuPtr cpu = NULL;
  xbt_assert(!surf_cpu_resource_priv(surf_cpu_resource_by_name(name)),
             "Host '%s' declared several times in the platform file",
             name);
  xbt_assert(power_peak > 0, "Power has to be >0");
  xbt_assert(core > 0, "Invalid number of cores %d", core);

  cpu = new CpuCas01Lmm(this, name, power_peak, power_scale, power_trace, core, state_initial, state_trace, cpu_properties);
  xbt_lib_set(host_lib, name, SURF_CPU_LEVEL, cpu);

  return (CpuCas01LmmPtr) xbt_lib_get_elm_or_null(host_lib, name);;
}

double CpuCas01Model::shareResourcesFull(double now)
{
  CpuCas01ActionLmm action;
  Model::shareResourcesMaxMin(p_runningActionSet, 
		             xbt_swag_offset(action, p_variable),
                             p_maxminSystem, lmm_solve);
  return 0;
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
    CpuCas01LmmPtr host = (CpuCas01LmmPtr) surf_cpu_resource_priv(surf_cpu_resource_by_name(elm));

    xbt_assert(host, "Host %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    host->p_stateEvent =
        tmgr_history_add_trace(history, trace, 0.0, 0, (ResourcePtr) host);
  }

  xbt_dict_foreach(trace_connect_list_power, cursor, trace_name, elm) {
    tmgr_trace_t trace = (tmgr_trace_t) xbt_dict_get_or_null(traces_set_list, trace_name);
    CpuCas01LmmPtr host = (CpuCas01LmmPtr) surf_cpu_resource_priv(surf_cpu_resource_by_name(elm));

    xbt_assert(host, "Host %s undefined", elm);
    xbt_assert(trace, "Trace %s undefined", trace_name);

    host->p_powerEvent =
        tmgr_history_add_trace(history, trace, 0.0, 0, (ResourcePtr) host);
  }
}

/************
 * Resource *
 ************/
CpuCas01Lmm::CpuCas01Lmm(CpuCas01ModelPtr model, const char *name, double powerPeak,
        double powerScale, tmgr_trace_t powerTrace, int core,
        e_surf_resource_state_t stateInitial, tmgr_trace_t stateTrace,
	xbt_dict_t properties) :
	CpuLmm(model, name, properties), Resource(model, name, properties) {
  m_powerPeak = powerPeak;
  m_powerScale = powerScale;
  m_core = core;
  p_stateCurrent = stateInitial;
  if (powerTrace)
    p_powerEvent = tmgr_history_add_trace(history, powerTrace, 0.0, 0, static_cast<ResourcePtr>(this));

  if (stateTrace)
    p_stateEvent = tmgr_history_add_trace(history, stateTrace, 0.0, 0, static_cast<ResourcePtr>(this));

  p_constraint = lmm_constraint_new(p_model->p_maxminSystem, this, m_core * m_powerScale * m_powerPeak);
}

bool CpuCas01Lmm::isUsed()
{
  return lmm_constraint_used(p_model->p_maxminSystem, p_constraint);
}

void CpuCas01Lmm::updateState(tmgr_trace_event_t event_type, double value, double date)
{
  lmm_variable_t var = NULL;
  lmm_element_t elem = NULL;

  surf_watched_hosts();

  if (event_type == p_powerEvent) {
    m_powerScale = value;
    lmm_update_constraint_bound(surf_cpu_model->p_maxminSystem, p_constraint,
                                m_core * m_powerScale *
                                m_powerPeak);
#ifdef HAVE_TRACING
    TRACE_surf_host_set_power(date, m_name,
                              m_core * m_powerScale *
                              m_powerPeak);
#endif
    while ((var = lmm_get_var_from_cnst
            (surf_cpu_model->p_maxminSystem, p_constraint, &elem))) {
      CpuCas01ActionLmmPtr action = static_cast<CpuCas01ActionLmmPtr>(static_cast<ActionLmmPtr>(lmm_variable_id(var)));

      lmm_update_variable_bound(surf_cpu_model->p_maxminSystem,
                                action->p_variable,
                                m_powerScale * m_powerPeak);
    }
    if (tmgr_trace_event_free(event_type))
      p_powerEvent = NULL;
  } else if (event_type == p_stateEvent) {
    if (value > 0)
      p_stateCurrent = SURF_RESOURCE_ON;
    else {
      lmm_constraint_t cnst = p_constraint;

      p_stateCurrent = SURF_RESOURCE_OFF;

      while ((var = lmm_get_var_from_cnst(surf_cpu_model->p_maxminSystem, cnst, &elem))) {
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

CpuActionPtr CpuCas01Lmm::execute(double size)
{

  XBT_IN("(%s,%g)", m_name, size);
  CpuCas01ActionLmmPtr action = new CpuCas01ActionLmm(surf_cpu_model, size, p_stateCurrent != SURF_RESOURCE_ON);

  action->m_suspended = 0;     /* Should be useless because of the
                                                   calloc but it seems to help valgrind... */

  action->p_variable =
      lmm_variable_new(surf_cpu_model->p_maxminSystem, static_cast<ActionLmmPtr>(action),
                       action->m_priority,
                       m_powerScale * m_powerPeak, 1);
  if (surf_cpu_model->p_updateMechanism == UM_LAZY) {
    action->m_indexHeap = -1;
    action->m_lastUpdate = surf_get_clock();
    action->m_lastValue = 0.0;
  }
  lmm_expand(surf_cpu_model->p_maxminSystem, p_constraint,
             action->p_variable, 1.0);
  XBT_OUT();
  return action;
}

CpuActionPtr CpuCas01Lmm::sleep(double duration)
{
  if (duration > 0)
    duration = MAX(duration, MAXMIN_PRECISION);

  XBT_IN("(%s,%g)", m_name, duration);
  CpuCas01ActionLmmPtr action = (CpuCas01ActionLmmPtr) execute(1.0);

  // FIXME: sleep variables should not consume 1.0 in lmm_expand
  action->m_maxDuration = duration;
  action->m_suspended = 2;
  if (duration == NO_MAX_DURATION) {
    /* Move to the *end* of the corresponding action set. This convention
       is used to speed up update_resource_state  */
    xbt_swag_remove(static_cast<ActionPtr>(action), action->p_stateSet);
    action->p_stateSet = cpu_running_action_set_that_does_not_need_being_checked;
    xbt_swag_insert(static_cast<ActionPtr>(action), action->p_stateSet);
  }

  lmm_update_variable_weight(surf_cpu_model->p_maxminSystem,
                             action->p_variable, 0.0);
  if (surf_cpu_model->p_updateMechanism == UM_LAZY) {     // remove action from the heap
    action->heapRemove(surf_cpu_model->p_actionHeap);
    // this is necessary for a variable with weight 0 since such
    // variables are ignored in lmm and we need to set its max_duration
    // correctly at the next call to share_resources
    xbt_swag_insert_at_head(action, surf_cpu_model->p_modifiedSet);
  }

  XBT_OUT();
  return action;
}


/**********
 * Action *
 **********/


