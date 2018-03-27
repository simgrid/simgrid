/* Copyright (c) 2009-2018. The SimGrid Team. All rights reserved.          */

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
    setUpdateMechanism(Model::UpdateAlgo::Full);
  } else if (optim == "Lazy") {
    xbt_assert(select || xbt_cfg_is_default_value("cpu/maxmin-selective-update"),
               "You cannot disable cpu selective update when using the lazy update mechanism");
    setUpdateMechanism(Model::UpdateAlgo::Lazy);
    select = true;
  } else {
    xbt_die("Unsupported optimization (%s) for this model", optim.c_str());
  }

  set_maxmin_system(new simgrid::kernel::lmm::System(select));

  if (getUpdateMechanism() == Model::UpdateAlgo::Lazy)
    get_maxmin_system()->modified_set_ = new kernel::resource::Action::ModifiedSet();
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
    : Cpu(model, host, model->get_maxmin_system()->constraint_new(this, core * speedPerPstate->front()), speedPerPstate,
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
  return model()->get_maxmin_system()->constraint_used(constraint());
}

/** @brief take into account changes of speed (either load or max) */
void CpuCas01::onSpeedChange() {
  kernel::lmm::Variable* var = nullptr;
  const_lmm_element_t elem = nullptr;

  model()->get_maxmin_system()->update_constraint_bound(constraint(), coresAmount_ * speed_.scale * speed_.peak);
  while ((var = constraint()->get_variable(&elem))) {
    CpuCas01Action* action = static_cast<CpuCas01Action*>(var->get_id());

    model()->get_maxmin_system()->update_variable_bound(action->get_variable(),
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
      kernel::lmm::Constraint* cnst = constraint();
      kernel::lmm::Variable* var    = nullptr;
      const_lmm_element_t elem = nullptr;
      double date              = surf_get_clock();

      turnOff();

      while ((var = cnst->get_variable(&elem))) {
        kernel::resource::Action* action = static_cast<kernel::resource::Action*>(var->get_id());

        if (action->get_state() == kernel::resource::Action::State::running ||
            action->get_state() == kernel::resource::Action::State::ready ||
            action->get_state() == kernel::resource::Action::State::not_in_the_system) {
          action->set_finish_time(date);
          action->set_state(kernel::resource::Action::State::failed);
        }
      }
    }
    tmgr_trace_event_unref(&stateEvent_);

  } else {
    xbt_die("Unknown event!\n");
  }
}

/** @brief Start a new execution on this CPU lasting @param size flops and using one core */
CpuAction* CpuCas01::execution_start(double size)
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
  action->set_max_duration(duration);
  action->suspended_ = kernel::resource::Action::SuspendStates::sleeping;
  if (duration < 0) { // NO_MAX_DURATION
    /* Move to the *end* of the corresponding action set. This convention is used to speed up update_resource_state */
    simgrid::xbt::intrusive_erase(*action->get_state_set(), *action);
    action->state_set_ = &static_cast<CpuCas01Model*>(model())->cpuRunningActionSetThatDoesNotNeedBeingChecked_;
    action->get_state_set()->push_back(*action);
  }

  model()->get_maxmin_system()->update_variable_weight(action->get_variable(), 0.0);
  if (model()->getUpdateMechanism() == kernel::resource::Model::UpdateAlgo::Lazy) { // remove action from the heap
    action->heapRemove();
    // this is necessary for a variable with weight 0 since such variables are ignored in lmm and we need to set its
    // max_duration correctly at the next call to share_resources
    model()->get_modified_set()->push_front(*action);
  }

  XBT_OUT();
  return action;
}

/**********
 * Action *
 **********/
CpuCas01Action::CpuCas01Action(kernel::resource::Model* model, double cost, bool failed, double speed,
                               kernel::lmm::Constraint* constraint, int requestedCore)
    : CpuAction(model, cost, failed,
                model->get_maxmin_system()->variable_new(this, 1.0 / requestedCore, requestedCore * speed, 1))
    , requestedCore_(requestedCore)
{
  if (model->getUpdateMechanism() == kernel::resource::Model::UpdateAlgo::Lazy) {
    set_last_update();
    set_last_value(0.0);
  }
  model->get_maxmin_system()->expand(constraint, get_variable(), 1.0);
}

CpuCas01Action::CpuCas01Action(kernel::resource::Model* model, double cost, bool failed, double speed,
                               kernel::lmm::Constraint* constraint)
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
