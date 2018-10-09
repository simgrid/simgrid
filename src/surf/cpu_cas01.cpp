/* Copyright (c) 2009-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "cpu_cas01.hpp"
#include "simgrid/sg_config.hpp"
#include "src/surf/cpu_ti.hpp"
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
  xbt_assert(surf_cpu_model_pm == nullptr, "CPU model already initialized. This should not happen.");
  xbt_assert(surf_cpu_model_vm == nullptr, "CPU model already initialized. This should not happen.");

  if (cpu_optim_opt == "TI") {
    simgrid::surf::CpuTiModel::create_pm_vm_models();
    return;
  }

  simgrid::kernel::resource::Model::UpdateAlgo algo;
  if (cpu_optim_opt == "Lazy")
    algo = simgrid::kernel::resource::Model::UpdateAlgo::LAZY;
  else
    algo = simgrid::kernel::resource::Model::UpdateAlgo::FULL;

  surf_cpu_model_pm = new simgrid::surf::CpuCas01Model(algo);
  surf_cpu_model_vm = new simgrid::surf::CpuCas01Model(algo);
}

namespace simgrid {
namespace surf {

CpuCas01Model::CpuCas01Model(kernel::resource::Model::UpdateAlgo algo) : simgrid::surf::CpuModel(algo)
{
  all_existing_models.push_back(this);

  bool select = simgrid::config::get_value<bool>("cpu/maxmin-selective-update");

  if (algo == Model::UpdateAlgo::LAZY) {
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

Cpu* CpuCas01Model::create_cpu(simgrid::s4u::Host* host, std::vector<double>* speed_per_pstate, int core)
{
  return new CpuCas01(this, host, speed_per_pstate, core);
}

/************
 * Resource *
 ************/
CpuCas01::CpuCas01(CpuCas01Model* model, simgrid::s4u::Host* host, std::vector<double>* speedPerPstate, int core)
    : Cpu(model, host, model->get_maxmin_system()->constraint_new(this, core * speedPerPstate->front()), speedPerPstate,
          core)
{
}

CpuCas01::~CpuCas01() = default;

bool CpuCas01::is_used()
{
  return get_model()->get_maxmin_system()->constraint_used(get_constraint());
}

/** @brief take into account changes of speed (either load or max) */
void CpuCas01::on_speed_change()
{
  kernel::lmm::Variable* var = nullptr;
  const kernel::lmm::Element* elem = nullptr;

  get_model()->get_maxmin_system()->update_constraint_bound(get_constraint(),
                                                            get_core_count() * speed_.scale * speed_.peak);
  while ((var = get_constraint()->get_variable(&elem))) {
    CpuCas01Action* action = static_cast<CpuCas01Action*>(var->get_id());

    get_model()->get_maxmin_system()->update_variable_bound(action->get_variable(),
                                                            action->requested_core() * speed_.scale * speed_.peak);
  }

  Cpu::on_speed_change();
}

void CpuCas01::apply_event(tmgr_trace_event_t event, double value)
{
  if (event == speed_.event) {
    /* TODO (Hypervisor): do the same thing for constraint_core[i] */
    xbt_assert(get_core_count() == 1, "FIXME: add speed scaling code also for constraint_core[i]");

    speed_.scale = value;
    on_speed_change();

    tmgr_trace_event_unref(&speed_.event);
  } else if (event == state_event_) {
    /* TODO (Hypervisor): do the same thing for constraint_core[i] */
    xbt_assert(get_core_count() == 1, "FIXME: add state change code also for constraint_core[i]");

    if (value > 0) {
      if (is_off()) {
        XBT_VERB("Restart processes on host %s", get_host()->get_cname());
        get_host()->turn_on();
      }
    } else {
      kernel::lmm::Constraint* cnst = get_constraint();
      kernel::lmm::Variable* var    = nullptr;
      const kernel::lmm::Element* elem = nullptr;
      double date              = surf_get_clock();

      get_host()->turn_off();

      while ((var = cnst->get_variable(&elem))) {
        kernel::resource::Action* action = static_cast<kernel::resource::Action*>(var->get_id());

        if (action->get_state() == kernel::resource::Action::State::INITED ||
            action->get_state() == kernel::resource::Action::State::STARTED ||
            action->get_state() == kernel::resource::Action::State::IGNORED) {
          action->set_finish_time(date);
          action->set_state(kernel::resource::Action::State::FAILED);
        }
      }
    }
    tmgr_trace_event_unref(&state_event_);

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

CpuAction* CpuCas01::sleep(double duration)
{
  if (duration > 0)
    duration = std::max(duration, sg_surf_precision);

  XBT_IN("(%s,%g)", get_cname(), duration);
  CpuCas01Action* action = new CpuCas01Action(get_model(), 1.0, is_off(), speed_.scale * speed_.peak, get_constraint());

  // FIXME: sleep variables should not consume 1.0 in System::expand()
  action->set_max_duration(duration);
  action->suspended_ = kernel::resource::Action::SuspendStates::sleeping;
  if (duration < 0) // NO_MAX_DURATION
    action->set_state(simgrid::kernel::resource::Action::State::IGNORED);

  get_model()->get_maxmin_system()->update_variable_weight(action->get_variable(), 0.0);
  if (get_model()->get_update_algorithm() == kernel::resource::Model::UpdateAlgo::LAZY) { // remove action from the heap
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
  if (model->get_update_algorithm() == kernel::resource::Model::UpdateAlgo::LAZY)
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
