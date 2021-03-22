/* Copyright (c) 2009-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "cpu_cas01.hpp"
#include "simgrid/kernel/routing/NetZoneImpl.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/sg_config.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/resource/profile/Event.hpp"
#include "src/surf/cpu_ti.hpp"
#include "src/surf/surf_interface.hpp"
#include "surf/surf.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(cpu_cas, res_cpu, "CPU resource, CAS01 model (used by default)");

/***********
 * Options *
 ***********/

static simgrid::config::Flag<std::string>
    cpu_optim_opt("cpu/optim", "Optimization algorithm to use for CPU resources. ", "Lazy",

                  std::map<std::string, std::string, std::less<>>({
                      {"Lazy", "Lazy action management (partial invalidation in lmm + heap in action remaining)."},
                      {"TI", "Trace integration. Highly optimized mode when using availability traces (only available "
                             "for the Cas01 CPU model for now)."},
                      {"Full", "Full update of remaining and variables. Slow but may be useful when debugging."},
                  }),

                  [](std::string const&) {
                    xbt_assert(_sg_cfg_init_status < 2,
                               "Cannot change the optimization algorithm after the initialization");
                  });

/*********
 * Model *
 *********/
void surf_cpu_model_init_Cas01()
{
  if (cpu_optim_opt == "TI") {
    simgrid::kernel::resource::CpuTiModel::create_pm_models();
    return;
  }

  auto cpu_model_pm = std::make_shared<simgrid::kernel::resource::CpuCas01Model>();
  simgrid::kernel::EngineImpl::get_instance()->add_model(simgrid::kernel::resource::Model::Type::CPU_PM, cpu_model_pm,
                                                         true);
  simgrid::s4u::Engine::get_instance()->get_netzone_root()->get_impl()->set_cpu_pm_model(cpu_model_pm);
}

namespace simgrid {
namespace kernel {
namespace resource {

CpuCas01Model::CpuCas01Model()
{
  if (config::get_value<std::string>("cpu/optim") == "Lazy")
    set_update_algorithm(Model::UpdateAlgo::LAZY);

  bool select = config::get_value<bool>("cpu/maxmin-selective-update");

  if (is_update_lazy()) {
    xbt_assert(select || config::is_default("cpu/maxmin-selective-update"),
               "You cannot disable cpu selective update when using the lazy update mechanism");
    select = true;
  }

  set_maxmin_system(new lmm::System(select));
}

Cpu* CpuCas01Model::create_cpu(s4u::Host* host, const std::vector<double>& speed_per_pstate)
{
  return (new CpuCas01(host, speed_per_pstate))->set_model(this);
}

/************
 * Resource *
 ************/
bool CpuCas01::is_used() const
{
  return get_model()->get_maxmin_system()->constraint_used(get_constraint());
}

/** @brief take into account changes of speed (either load or max) */
void CpuCas01::on_speed_change()
{
  const lmm::Element* elem = nullptr;

  get_model()->get_maxmin_system()->update_constraint_bound(get_constraint(),
                                                            get_core_count() * speed_.scale * speed_.peak);
  while (const auto* var = get_constraint()->get_variable(&elem)) {
    const auto* action = static_cast<CpuCas01Action*>(var->get_id());

    get_model()->get_maxmin_system()->update_variable_bound(action->get_variable(),
                                                            action->requested_core() * speed_.scale * speed_.peak);
  }

  Cpu::on_speed_change();
}

void CpuCas01::apply_event(profile::Event* event, double value)
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
      if (not is_on()) {
        XBT_VERB("Restart actors on host %s", get_iface()->get_cname());
        get_iface()->turn_on();
      }
    } else {
      const lmm::Element* elem = nullptr;
      double date              = surf_get_clock();

      get_iface()->turn_off();

      while (const auto* var = get_constraint()->get_variable(&elem)) {
        Action* action = var->get_id();

        if (action->get_state() == Action::State::INITED || action->get_state() == Action::State::STARTED ||
            action->get_state() == Action::State::IGNORED) {
          action->set_finish_time(date);
          action->set_state(Action::State::FAILED);
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
  return new CpuCas01Action(get_model(), size, not is_on(), speed_.scale * speed_.peak, get_constraint());
}

CpuAction* CpuCas01::execution_start(double size, int requested_cores)
{
  return new CpuCas01Action(get_model(), size, not is_on(), speed_.scale * speed_.peak, get_constraint(),
                            requested_cores);
}

CpuAction* CpuCas01::sleep(double duration)
{
  if (duration > 0)
    duration = std::max(duration, sg_surf_precision);

  XBT_IN("(%s,%g)", get_cname(), duration);
  auto* action = new CpuCas01Action(get_model(), 1.0, not is_on(), speed_.scale * speed_.peak, get_constraint());

  // FIXME: sleep variables should not consume 1.0 in System::expand()
  action->set_max_duration(duration);
  action->set_suspend_state(Action::SuspendStates::SLEEPING);
  if (duration == NO_MAX_DURATION)
    action->set_state(Action::State::IGNORED);

  get_model()->get_maxmin_system()->update_variable_penalty(action->get_variable(), 0.0);
  if (get_model()->is_update_lazy()) { // remove action from the heap
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
CpuCas01Action::CpuCas01Action(Model* model, double cost, bool failed, double speed, lmm::Constraint* constraint,
                               int requested_core)
    : CpuAction(model, cost, failed,
                model->get_maxmin_system()->variable_new(this, 1.0 / requested_core, requested_core * speed, 1))
    , requested_core_(requested_core)
{
  if (model->is_update_lazy())
    set_last_update();
  model->get_maxmin_system()->expand(constraint, get_variable(), 1.0);
}

int CpuCas01Action::requested_core() const
{
  return requested_core_;
}

} // namespace resource
} // namespace kernel
} // namespace simgrid
