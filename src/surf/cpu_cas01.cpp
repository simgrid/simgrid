/* Copyright (c) 2009-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/kernel/routing/NetZoneImpl.hpp>
#include <simgrid/s4u/Engine.hpp>

#include "simgrid/sg_config.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/resource/profile/Event.hpp"
#include "src/surf/cpu_cas01.hpp"
#include "src/surf/cpu_ti.hpp"
#include "src/surf/surf_interface.hpp"

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

static simgrid::config::Flag<std::string> cfg_cpu_solver("cpu/solver", "Set linear equations solver used by CPU model",
                                                         "maxmin", &simgrid::kernel::lmm::System::validate_solver);

/*********
 * Model *
 *********/
void surf_cpu_model_init_Cas01()
{
  if (cpu_optim_opt == "TI") {
    simgrid::kernel::resource::CpuTiModel::create_pm_models();
    return;
  }

  auto cpu_model_pm = std::make_shared<simgrid::kernel::resource::CpuCas01Model>("Cpu_Cas01");
  auto* engine      = simgrid::kernel::EngineImpl::get_instance();
  engine->add_model(cpu_model_pm);
  engine->get_netzone_root()->set_cpu_pm_model(cpu_model_pm);
}

namespace simgrid {
namespace kernel {
namespace resource {

CpuCas01Model::CpuCas01Model(const std::string& name) : CpuModel(name)
{
  if (config::get_value<std::string>("cpu/optim") == "Lazy")
    set_update_algorithm(Model::UpdateAlgo::LAZY);

  bool select = config::get_value<bool>("cpu/maxmin-selective-update");

  if (is_update_lazy()) {
    xbt_assert(select || config::is_default("cpu/maxmin-selective-update"),
               "You cannot disable cpu selective update when using the lazy update mechanism");
    select = true;
  }

  set_maxmin_system(lmm::System::build(cfg_cpu_solver, select));
}

CpuImpl* CpuCas01Model::create_cpu(s4u::Host* host, const std::vector<double>& speed_per_pstate)
{
  return (new CpuCas01(host, speed_per_pstate))->set_model(this);
}

/************
 * Resource *
 ************/
/** @brief take into account changes of speed (either load or max) */
void CpuCas01::on_speed_change()
{
  const lmm::Element* elem = nullptr;

  get_model()->get_maxmin_system()->update_constraint_bound(get_constraint(),
                                                            get_core_count() * speed_.scale * speed_.peak);

  while (const auto* var = get_constraint()->get_variable(&elem)) {
    const auto* action = static_cast<CpuCas01Action*>(var->get_id());
    double bound       = action->requested_core() * speed_.scale * speed_.peak;
    if (action->get_user_bound() > 0) {
      bound = std::min(bound, action->get_user_bound());
    }

    get_model()->get_maxmin_system()->update_variable_bound(action->get_variable(), bound);
  }

  CpuImpl::on_speed_change();
}

void CpuCas01::apply_event(profile::Event* event, double value)
{
  if (event == speed_.event) {
    speed_.scale = value;
    on_speed_change();

    tmgr_trace_event_unref(&speed_.event);
  } else if (event == get_state_event()) {
    if (value > 0) {
      if (not is_on()) {
        XBT_VERB("Restart actors on host %s", get_iface()->get_cname());
        get_iface()->turn_on();
      }
    } else {
      const lmm::Element* elem = nullptr;
      double date              = EngineImpl::get_clock();

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
    unref_state_event();

  } else {
    xbt_die("Unknown event!\n");
  }
}

/** @brief Start a new execution on this CPU lasting @param size flops and using one core */
CpuAction* CpuCas01::execution_start(double size, double user_bound)
{
  return execution_start(size, 1, user_bound);
}

CpuAction* CpuCas01::execution_start(double size, int requested_cores, double user_bound)
{
  auto* action =
      new CpuCas01Action(get_model(), size, not is_on(), speed_.scale * speed_.peak, get_constraint(), requested_cores);
  action->set_user_bound(user_bound);
  if (user_bound > 0 && user_bound < action->get_bound()) {
    get_model()->get_maxmin_system()->update_variable_bound(action->get_variable(), user_bound);
  }
  if (factor_cb_) {
    action->set_rate_factor(factor_cb_(size));
  }

  return action;
}

CpuAction* CpuCas01::sleep(double duration)
{
  if (duration > 0)
    duration = std::max(duration, sg_surf_precision);

  XBT_IN("(%s, %g)", get_cname(), duration);
  auto* action = new CpuCas01Action(get_model(), 1.0, not is_on(), speed_.scale * speed_.peak, get_constraint(), 1);

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

void CpuCas01::set_factor_cb(const std::function<s4u::Host::CpuFactorCb>& cb)
{
  xbt_assert(not is_sealed(), "Cannot set CPU factor callback in an already sealed CPU(%s)", get_cname());
  factor_cb_ = cb;
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

} // namespace resource
} // namespace kernel
} // namespace simgrid
