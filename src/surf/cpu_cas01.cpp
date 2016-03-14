/* Copyright (c) 2009-2011, 2013-2016. The SimGrid Team.
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
  xbt_assert(!surf_cpu_model_pm);
  xbt_assert(!surf_cpu_model_vm);

  char *optim = xbt_cfg_get_string(_sg_cfg_set, "cpu/optim");
  if (!strcmp(optim, "TI")) {
    surf_cpu_model_init_ti();
    return;
  }

  surf_cpu_model_pm = new simgrid::surf::CpuCas01Model();
  xbt_dynar_push(all_existing_models, &surf_cpu_model_pm);

  surf_cpu_model_vm  = new simgrid::surf::CpuCas01Model();
  xbt_dynar_push(all_existing_models, &surf_cpu_model_vm);
}

namespace simgrid {
namespace surf {

CpuCas01Model::CpuCas01Model() : simgrid::surf::CpuModel()
{
  char *optim = xbt_cfg_get_string(_sg_cfg_set, "cpu/optim");
  int select = xbt_cfg_get_boolean(_sg_cfg_set, "cpu/maxmin_selective_update");

  if (!strcmp(optim, "Full")) {
    updateMechanism_ = UM_FULL;
    selectiveUpdate_ = select;
  } else if (!strcmp(optim, "Lazy")) {
    updateMechanism_ = UM_LAZY;
    selectiveUpdate_ = 1;
    xbt_assert((select == 1)
               ||
               (xbt_cfg_is_default_value
                (_sg_cfg_set, "cpu/maxmin_selective_update")),
               "Disabling selective update while using the lazy update mechanism is dumb!");
  } else {
    xbt_die("Unsupported optimization (%s) for this model", optim);
  }

  p_cpuRunningActionSetThatDoesNotNeedBeingChecked = new ActionList();

  maxminSystem_ = lmm_system_new(selectiveUpdate_);

  if (getUpdateMechanism() == UM_LAZY) {
    actionHeap_ = xbt_heap_new(8, NULL);
    xbt_heap_set_update_callback(actionHeap_,  surf_action_lmm_update_index_heap);
    modifiedSet_ = new ActionLmmList();
    maxminSystem_->keep_track = modifiedSet_;
  }
}

CpuCas01Model::~CpuCas01Model()
{
  lmm_system_free(maxminSystem_);
  maxminSystem_ = NULL;

  if (actionHeap_)
    xbt_heap_free(actionHeap_);
  delete modifiedSet_;

  surf_cpu_model_pm = NULL;

  delete p_cpuRunningActionSetThatDoesNotNeedBeingChecked;
}

Cpu *CpuCas01Model::createCpu(simgrid::s4u::Host *host, xbt_dynar_t speedPeak,
     tmgr_trace_t speedTrace, int core, tmgr_trace_t state_trace)
{
  xbt_assert(xbt_dynar_getfirst_as(speedPeak, double) > 0.0,
      "Speed has to be >0.0. Did you forget to specify the mandatory power attribute?");
  xbt_assert(core > 0, "Invalid number of cores %d. Must be larger than 0", core);
  Cpu *cpu = new CpuCas01(this, host, speedPeak, speedTrace, core, state_trace);
  return cpu;
}

double CpuCas01Model::next_occuring_event_full(double /*now*/)
{
  return Model::shareResourcesMaxMin(getRunningActionSet(), maxminSystem_, lmm_solve);
}

/************
 * Resource *
 ************/
CpuCas01::CpuCas01(CpuCas01Model *model, simgrid::s4u::Host *host, xbt_dynar_t speedPeak,
    tmgr_trace_t speedTrace, int core,  tmgr_trace_t stateTrace)
: Cpu(model, host,
    lmm_constraint_new(model->getMaxminSystem(), this, core * xbt_dynar_get_as(speedPeak, 0/*pstate*/, double)),
    speedPeak, core, xbt_dynar_get_as(speedPeak, 0/*pstate*/, double))
{

  XBT_DEBUG("CPU create: peak=%f, pstate=%d", speed_.peak, pstate_);

  coresAmount_ = core;
  if (speedTrace)
    speed_.event = future_evt_set->add_trace(speedTrace, 0.0, this);

  if (stateTrace)
    stateEvent_ = future_evt_set->add_trace(stateTrace, 0.0, this);
}

CpuCas01::~CpuCas01()
{
  if (getModel() == surf_cpu_model_pm)
    xbt_dynar_free(&speedPeakList_);
}

xbt_dynar_t CpuCas01::getSpeedPeakList(){
  return speedPeakList_;
}

bool CpuCas01::isUsed()
{
  return lmm_constraint_used(getModel()->getMaxminSystem(), getConstraint());
}

/** @brief take into account changes of speed (either load or max) */
void CpuCas01::onSpeedChange() {
  lmm_variable_t var = NULL;
  lmm_element_t elem = NULL;

    lmm_update_constraint_bound(getModel()->getMaxminSystem(), getConstraint(),
                                coresAmount_ * speed_.scale * speed_.peak);
    while ((var = lmm_get_var_from_cnst
            (getModel()->getMaxminSystem(), getConstraint(), &elem))) {
      CpuCas01Action *action = static_cast<CpuCas01Action*>(lmm_variable_id(var));

      lmm_update_variable_bound(getModel()->getMaxminSystem(),
                                action->getVariable(),
                                speed_.scale * speed_.peak);
    }

  Cpu::onSpeedChange();
}

void CpuCas01::apply_event(tmgr_trace_iterator_t event, double value)
{
  if (event == speed_.event) {
    /* TODO (Hypervisor): do the same thing for constraint_core[i] */
    xbt_assert(coresAmount_ == 1, "FIXME: add speed scaling code also for constraint_core[i]");

    speed_.scale = value;
    onSpeedChange();

    tmgr_trace_event_unref(&speed_.event);
  } else if (event == stateEvent_) {
    /* TODO (Hypervisor): do the same thing for constraint_core[i] */
    xbt_assert(coresAmount_ == 1, "FIXME: add state change code also for constraint_core[i]");

    if (value > 0) {
      if(isOff())
        xbt_dynar_push_as(host_that_restart, char*, (char *)getName());
      turnOn();
    } else {
      lmm_constraint_t cnst = getConstraint();
      lmm_variable_t var = NULL;
      lmm_element_t elem = NULL;
      double date = surf_get_clock();

      turnOff();

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
    tmgr_trace_event_unref(&stateEvent_);

  } else {
    xbt_die("Unknown event!\n");
  }
}

CpuAction *CpuCas01::execution_start(double size)
{

  XBT_IN("(%s,%g)", getName(), size);
  CpuCas01Action *action = new CpuCas01Action(getModel(), size, isOff(),
      speed_.scale * speed_.peak, getConstraint());

  XBT_OUT();
  return action;
}

CpuAction *CpuCas01::sleep(double duration)
{
  if (duration > 0)
    duration = MAX(duration, sg_surf_precision);

  XBT_IN("(%s,%g)", getName(), duration);
  CpuCas01Action *action = new CpuCas01Action(getModel(), 1.0, isOff(),
      speed_.scale * speed_.peak, getConstraint());


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

/**********
 * Action *
 **********/

CpuCas01Action::CpuCas01Action(Model *model, double cost, bool failed, double speed, lmm_constraint_t constraint)
 : CpuAction(model, cost, failed,
         lmm_variable_new(model->getMaxminSystem(), this,
         1.0, speed, 1))
{
  if (model->getUpdateMechanism() == UM_LAZY) {
    m_indexHeap = -1;
    m_lastUpdate = surf_get_clock();
    m_lastValue = 0.0;
  }
  lmm_expand(model->getMaxminSystem(), constraint, getVariable(), 1.0);
}

}
}
