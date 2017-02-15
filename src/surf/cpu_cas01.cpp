/* Copyright (c) 2009-2011, 2013-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "cpu_cas01.hpp"
#include "cpu_ti.hpp"
#include "maxmin_private.hpp"
#include "simgrid/sg_config.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_cpu_cas, surf_cpu, "Logging specific to the SURF CPU IMPROVED module");

/*********
 * Model *
 *********/
void surf_cpu_model_init_Cas01()
{
  xbt_assert(!surf_cpu_model_pm);
  xbt_assert(!surf_cpu_model_vm);

  char *optim = xbt_cfg_get_string("cpu/optim");
  if (!strcmp(optim, "TI")) {
    surf_cpu_model_init_ti();
    return;
  }

  surf_cpu_model_pm = new simgrid::surf::CpuCas01Model();
  all_existing_models->push_back(surf_cpu_model_pm);

  surf_cpu_model_vm  = new simgrid::surf::CpuCas01Model();
  all_existing_models->push_back(surf_cpu_model_vm);
}

namespace simgrid {
namespace surf {

CpuCas01Model::CpuCas01Model() : simgrid::surf::CpuModel()
{
  char *optim = xbt_cfg_get_string("cpu/optim");
  bool select = xbt_cfg_get_boolean("cpu/maxmin-selective-update");

  if (!strcmp(optim, "Full")) {
    updateMechanism_ = UM_FULL;
    selectiveUpdate_ = select;
  } else if (!strcmp(optim, "Lazy")) {
    updateMechanism_ = UM_LAZY;
    selectiveUpdate_ = true;
    xbt_assert(select || (xbt_cfg_is_default_value("cpu/maxmin-selective-update")),
               "Disabling selective update while using the lazy update mechanism is dumb!");
  } else {
    xbt_die("Unsupported optimization (%s) for this model", optim);
  }

  p_cpuRunningActionSetThatDoesNotNeedBeingChecked = new ActionList();
  maxminSystem_ = lmm_system_new(selectiveUpdate_);

  if (getUpdateMechanism() == UM_LAZY) {
    actionHeap_ = xbt_heap_new(8, nullptr);
    xbt_heap_set_update_callback(actionHeap_,  surf_action_lmm_update_index_heap);
    modifiedSet_ = new ActionLmmList();
    maxminSystem_->keep_track = modifiedSet_;
  }
}

CpuCas01Model::~CpuCas01Model()
{
  lmm_system_free(maxminSystem_);
  maxminSystem_ = nullptr;
  xbt_heap_free(actionHeap_);
  delete modifiedSet_;

  surf_cpu_model_pm = nullptr;

  delete p_cpuRunningActionSetThatDoesNotNeedBeingChecked;
}

Cpu *CpuCas01Model::createCpu(simgrid::s4u::Host *host, std::vector<double> *speedPerPstate, int core)
{
  return new CpuCas01(this, host, speedPerPstate, core);
}

/************
 * Resource *
 ************/
CpuCas01::CpuCas01(CpuCas01Model *model, simgrid::s4u::Host *host, std::vector<double> *speedPerPstate, int core)
: Cpu(model, host, lmm_constraint_new(model->getMaxminSystem(), this, core * speedPerPstate->front()),
    speedPerPstate, core)
{
}

CpuCas01::~CpuCas01()
{
  if (model() == surf_cpu_model_pm)
    speedPerPstate_.clear();
}

std::vector<double> * CpuCas01::getSpeedPeakList(){
  return &speedPerPstate_;
}

bool CpuCas01::isUsed()
{
  return lmm_constraint_used(model()->getMaxminSystem(), constraint());
}

/** @brief take into account changes of speed (either load or max) */
void CpuCas01::onSpeedChange() {
  lmm_variable_t var = nullptr;
  lmm_element_t elem = nullptr;

  lmm_update_constraint_bound(model()->getMaxminSystem(), constraint(), coresAmount_ * speed_.scale * speed_.peak);
  while ((var = lmm_get_var_from_cnst(model()->getMaxminSystem(), constraint(), &elem))) {
    CpuCas01Action* action = static_cast<CpuCas01Action*>(lmm_variable_id(var));

    lmm_update_variable_bound(model()->getMaxminSystem(), action->getVariable(), speed_.scale * speed_.peak);
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
        host_that_restart.push_back(getHost());
      turnOn();
    } else {
      lmm_constraint_t cnst = constraint();
      lmm_variable_t var = nullptr;
      lmm_element_t elem = nullptr;
      double date = surf_get_clock();

      turnOff();

      while ((var = lmm_get_var_from_cnst(model()->getMaxminSystem(), cnst, &elem))) {
        Action *action = static_cast<Action*>(lmm_variable_id(var));

        if (action->getState() == Action::State::running ||
            action->getState() == Action::State::ready ||
            action->getState() == Action::State::not_in_the_system) {
          action->setFinishTime(date);
          action->setState(Action::State::failed);
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
  return new CpuCas01Action(model(), size, isOff(), speed_.scale * speed_.peak, constraint());
}

CpuAction *CpuCas01::sleep(double duration)
{
  if (duration > 0)
    duration = MAX(duration, sg_surf_precision);

  XBT_IN("(%s,%g)", cname(), duration);
  CpuCas01Action* action = new CpuCas01Action(model(), 1.0, isOff(), speed_.scale * speed_.peak, constraint());

  // FIXME: sleep variables should not consume 1.0 in lmm_expand
  action->maxDuration_ = duration;
  action->suspended_ = 2;
  if (duration == NO_MAX_DURATION) {
    /* Move to the *end* of the corresponding action set. This convention is used to speed up update_resource_state */
    action->getStateSet()->erase(action->getStateSet()->iterator_to(*action));
    action->stateSet_ = static_cast<CpuCas01Model*>(model())->p_cpuRunningActionSetThatDoesNotNeedBeingChecked;
    action->getStateSet()->push_back(*action);
  }

  lmm_update_variable_weight(model()->getMaxminSystem(), action->getVariable(), 0.0);
  if (model()->getUpdateMechanism() == UM_LAZY) { // remove action from the heap
    action->heapRemove(model()->getActionHeap());
    // this is necessary for a variable with weight 0 since such variables are ignored in lmm and we need to set its
    // max_duration correctly at the next call to share_resources
    model()->getModifiedSet()->push_front(*action);
  }

  XBT_OUT();
  return action;
}

/**********
 * Action *
 **********/

CpuCas01Action::CpuCas01Action(Model *model, double cost, bool failed, double speed, lmm_constraint_t constraint)
 : CpuAction(model, cost, failed, lmm_variable_new(model->getMaxminSystem(), this, 1.0, speed, 1))
{
  if (model->getUpdateMechanism() == UM_LAZY) {
    indexHeap_ = -1;
    lastUpdate_ = surf_get_clock();
    lastValue_ = 0.0;
  }
  lmm_expand(model->getMaxminSystem(), constraint, getVariable(), 1.0);
}

CpuCas01Action::~CpuCas01Action()=default;

}
}
