/* Copyright (c) 2009-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "cpu_cas01.hpp"
#include "simgrid/sg_config.hpp"
#include "src/surf/surf_interface.hpp"
#include "surf/surf.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_cpu_cas, surf_cpu, "Logging specific to the SURF CPU module");

/***********
 * Options *
 ***********/

static simgrid::config::Flag<std::string>
    cpu_optim_opt("cpu/optim", "Optimization algorithm to use for CPU resources. ", "Lazy",

                  std::map<std::string, std::string>({
                      {"Lazy", "Lazy action management (partial invalidation in lmm + heap in action remaining)."},
                      {"TI", "Trace integration. Highly optimized mode when using availability traces (only available "
                             "for the Cas01 CPU model for now)."},
                      {"Full", "Full update of remaining and variables. Slow but may be useful when debugging."},
                  }),

                  [](std::string const& val) {
                    xbt_assert(_sg_cfg_init_status < 2,
                               "Cannot change the optimization algorithm after the initialization");
                  });

/*********
 * Model *
 *********/
void surf_cpu_model_init_Cas01()
{
  xbt_assert(not surf_cpu_model_pm);
  xbt_assert(not surf_cpu_model_vm);

  if (cpu_optim_opt == "TI") {
    surf_cpu_model_init_ti();
    return;
  }

  simgrid::kernel::resource::Model::UpdateAlgo algo;
  if (cpu_optim_opt == "Lazy")
    algo = simgrid::kernel::resource::Model::UpdateAlgo::Lazy;
  else
    algo = simgrid::kernel::resource::Model::UpdateAlgo::Full;

  surf_cpu_model_pm = new simgrid::surf::CpuCas01Model(algo);
  all_existing_models->push_back(surf_cpu_model_pm);

  surf_cpu_model_vm = new simgrid::surf::CpuCas01Model(algo);
  all_existing_models->push_back(surf_cpu_model_vm);
}

namespace simgrid {
namespace surf {

CpuCas01Model::CpuCas01Model(kernel::resource::Model::UpdateAlgo algo) : simgrid::surf::CpuModel(algo)
{
  bool select = simgrid::config::get_value<bool>("cpu/maxmin-selective-update");

  if (algo == Model::UpdateAlgo::Lazy) {
    xbt_assert(select || simgrid::config::is_default("cpu/maxmin-selective-update"),
               "You cannot disable cpu selective update when using the lazy update mechanism");
    select = true;
  }

  set_maxmin_system(new simgrid::kernel::lmm::System(select));
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
  if (get_model() == surf_cpu_model_pm)
    speedPerPstate_.clear();
}

std::vector<double> * CpuCas01::getSpeedPeakList(){
  return &speedPerPstate_;
}

bool CpuCas01::is_used()
{
  return get_model()->get_maxmin_system()->constraint_used(get_constraint());
}

/** @brief take into account changes of speed (either load or max) */
void CpuCas01::onSpeedChange() {
  kernel::lmm::Variable* var = nullptr;
  const kernel::lmm::Element* elem = nullptr;

  get_model()->get_maxmin_system()->update_constraint_bound(get_constraint(),
                                                            coresAmount_ * speed_.scale * speed_.peak);
  while ((var = get_constraint()->get_variable(&elem))) {
    CpuCas01Action* action = static_cast<CpuCas01Action*>(var->get_id());

    get_model()->get_maxmin_system()->update_variable_bound(action->get_variable(),
                                                            action->requested_core() * speed_.scale * speed_.peak);
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
      if (is_off())
        host_that_restart.push_back(getHost());
      turn_on();
    } else {
      kernel::lmm::Constraint* cnst = get_constraint();
      kernel::lmm::Variable* var    = nullptr;
      const kernel::lmm::Element* elem = nullptr;
      double date              = surf_get_clock();

      turn_off();

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
  return new CpuCas01Action(get_model(), size, is_off(), speed_.scale * speed_.peak, get_constraint());
}

CpuAction* CpuCas01::execution_start(double size, int requested_cores)
{
  return new CpuCas01Action(get_model(), size, is_off(), speed_.scale * speed_.peak, get_constraint(), requested_cores);
}

CpuAction *CpuCas01::sleep(double duration)
{
  if (duration > 0)
    duration = std::max(duration, sg_surf_precision);

  XBT_IN("(%s,%g)", get_cname(), duration);
  CpuCas01Action* action = new CpuCas01Action(get_model(), 1.0, is_off(), speed_.scale * speed_.peak, get_constraint());

  // FIXME: sleep variables should not consume 1.0 in System::expand()
  action->set_max_duration(duration);
  action->suspended_ = kernel::resource::Action::SuspendStates::sleeping;
  if (duration < 0) { // NO_MAX_DURATION
    /* Move to the *end* of the corresponding action set. This convention is used to speed up update_resource_state */
    simgrid::xbt::intrusive_erase(*action->get_state_set(), *action);
    action->state_set_ = &static_cast<CpuCas01Model*>(get_model())->cpuRunningActionSetThatDoesNotNeedBeingChecked_;
    action->get_state_set()->push_back(*action);
  }

  get_model()->get_maxmin_system()->update_variable_weight(action->get_variable(), 0.0);
  if (get_model()->get_update_algorithm() == kernel::resource::Model::UpdateAlgo::Lazy) { // remove action from the heap
    get_model()->get_action_heap().remove(action);
    // this is necessary for a variable with weight 0 since such variables are ignored in lmm and we need to set its
    // max_duration correctly at the next call to share_resources
    get_model()->get_modified_set()->push_front(*action);
  }

  XBT_OUT();
  return action;
}

/**********
 * Action *
 **********/
CpuCas01Action::CpuCas01Action(kernel::resource::Model* model, double cost, bool failed, double speed,
                               kernel::lmm::Constraint* constraint, int requested_core)
    : CpuAction(model, cost, failed,
                model->get_maxmin_system()->variable_new(this, 1.0 / requested_core, requested_core * speed, 1))
    , requested_core_(requested_core)
{
  if (model->get_update_algorithm() == kernel::resource::Model::UpdateAlgo::Lazy)
    set_last_update();
  model->get_maxmin_system()->expand(constraint, get_variable(), 1.0);
}

CpuCas01Action::CpuCas01Action(kernel::resource::Model* model, double cost, bool failed, double speed,
                               kernel::lmm::Constraint* constraint)
    : CpuCas01Action(model, cost, failed, speed, constraint, /* requested_core */ 1)
{
}

int CpuCas01Action::requested_core()
{
  return requested_core_;
}

CpuCas01Action::~CpuCas01Action()=default;

}
}
