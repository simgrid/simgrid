/* Copyright (c) 2009-2011, 2013-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "cpu_cas01.hpp"
#include "cpu_ti.hpp"
#include "simgrid/sg_config.h"
#include "src/kernel/lmm/maxmin.hpp"
#include "xbt/utility.hpp"
#include <algorithm>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_cpu_cas, surf_cpu, "Logging specific to the SURF CPU IMPROVED module");

/*********
 * Model *
 *********/
void surf_cpu_model_init_Cas01()
{
  xbt_assert(not surf_cpu_model_pm);
  xbt_assert(not surf_cpu_model_vm);

  if (xbt_cfg_get_string("cpu/optim") == "TI") {
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
  std::string optim = xbt_cfg_get_string("cpu/optim");
  bool select = xbt_cfg_get_boolean("cpu/maxmin-selective-update");

  if (optim == "Full") {
    setUpdateMechanism(UM_FULL);
    selectiveUpdate_ = select;
  } else if (optim == "Lazy") {
    setUpdateMechanism(UM_LAZY);
    selectiveUpdate_ = true;
    xbt_assert(select || (xbt_cfg_is_default_value("cpu/maxmin-selective-update")),
               "Disabling selective update while using the lazy update mechanism is dumb!");
  } else {
    xbt_die("Unsupported optimization (%s) for this model", optim.c_str());
  }

  maxminSystem_ = new simgrid::kernel::lmm::System(selectiveUpdate_);

  if (getUpdateMechanism() == UM_LAZY) {
    modifiedSet_ = new ActionLmmList();
    maxminSystem_->keep_track = modifiedSet_;
  }
}

CpuCas01Model::~CpuCas01Model()
{
  surf_cpu_model_pm = nullptr;
}

Cpu *CpuCas01Model::createCpu(simgrid::s4u::Host *host, std::vector<double> *speedPerPstate, int core)
{
  return new CpuCas01(this, host, speedPerPstate, core);
}

/************
 * Resource *
 ************/
CpuCas01::CpuCas01(CpuCas01Model* model, simgrid::s4u::Host* host, std::vector<double>* speedPerPstate, int core)
    : Cpu(model, host, model->getMaxminSystem()->constraint_new(this, core * speedPerPstate->front()), speedPerPstate,
          core)
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
  return model()->getMaxminSystem()->constraint_used(constraint());
}

/** @brief take into account changes of speed (either load or max) */
void CpuCas01::onSpeedChange() {
  lmm_variable_t var       = nullptr;
  const_lmm_element_t elem = nullptr;

  model()->getMaxminSystem()->update_constraint_bound(constraint(), coresAmount_ * speed_.scale * speed_.peak);
  while ((var = constraint()->get_variable(&elem))) {
    CpuCas01Action* action = static_cast<CpuCas01Action*>(var->get_id());

    model()->getMaxminSystem()->update_variable_bound(action->getVariable(),
                                                      action->requestedCore() * speed_.scale * speed_.peak);
  }

  Cpu::onSpeedChange();
}

void CpuCas01::apply_event(tmgr_trace_event_t event, double value)
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
      lmm_constraint_t cnst    = constraint();
      lmm_variable_t var       = nullptr;
      const_lmm_element_t elem = nullptr;
      double date              = surf_get_clock();

      turnOff();

      while ((var = cnst->get_variable(&elem))) {
        Action* action = static_cast<Action*>(var->get_id());

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

/** @brief Start a new execution on this CPU lasting @param size flops and using one core */
CpuAction *CpuCas01::execution_start(double size)
{
  return new CpuCas01Action(model(), size, isOff(), speed_.scale * speed_.peak, constraint());
}
CpuAction* CpuCas01::execution_start(double size, int requestedCores)
{
  return new CpuCas01Action(model(), size, isOff(), speed_.scale * speed_.peak, constraint(), requestedCores);
}

CpuAction *CpuCas01::sleep(double duration)
{
  if (duration > 0)
    duration = std::max(duration, sg_surf_precision);

  XBT_IN("(%s,%g)", getCname(), duration);
  CpuCas01Action* action = new CpuCas01Action(model(), 1.0, isOff(), speed_.scale * speed_.peak, constraint());

  // FIXME: sleep variables should not consume 1.0 in System::expand()
  action->setMaxDuration(duration);
  action->suspended_ = 2;
  if (duration < 0) { // NO_MAX_DURATION
    /* Move to the *end* of the corresponding action set. This convention is used to speed up update_resource_state */
    simgrid::xbt::intrusive_erase(*action->getStateSet(), *action);
    action->stateSet_ = &static_cast<CpuCas01Model*>(model())->p_cpuRunningActionSetThatDoesNotNeedBeingChecked;
    action->getStateSet()->push_back(*action);
  }

  model()->getMaxminSystem()->update_variable_weight(action->getVariable(), 0.0);
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
CpuCas01Action::CpuCas01Action(Model* model, double cost, bool failed, double speed, lmm_constraint_t constraint,
                               int requestedCore)
    : CpuAction(model, cost, failed,
                model->getMaxminSystem()->variable_new(this, 1.0 / requestedCore, requestedCore * speed, 1))
    , requestedCore_(requestedCore)
{
  if (model->getUpdateMechanism() == UM_LAZY) {
    refreshLastUpdate();
    setLastValue(0.0);
  }
  model->getMaxminSystem()->expand(constraint, getVariable(), 1.0);
}

CpuCas01Action::CpuCas01Action(Model* model, double cost, bool failed, double speed, lmm_constraint_t constraint)
    : CpuCas01Action(model, cost, failed, speed, constraint, 1)
{
}

int CpuCas01Action::requestedCore()
{
  return requestedCore_;
}

CpuCas01Action::~CpuCas01Action()=default;

}
}
