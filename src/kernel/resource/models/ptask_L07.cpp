/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/kernel/routing/NetZoneImpl.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <xbt/asserts.hpp>
#include <xbt/config.hpp>

#include "simgrid/config.h"
#include "src/kernel/EngineImpl.hpp"
#include "src/simgrid/math_utils.h"
#include "src/simgrid/module.hpp"
#if SIMGRID_HAVE_EIGEN3
#include "src/kernel/lmm/bmf.hpp"
#endif
#include "src/kernel/resource/models/ptask_L07.hpp"
#include "src/kernel/resource/profile/Event.hpp"

#include <unordered_set>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(res_host);
XBT_LOG_EXTERNAL_CATEGORY(xbt_cfg);

/***********
 * Options *
 ***********/
static simgrid::config::Flag<std::string> cfg_ptask_solver("host/solver",
                                                           "Set linear equations solver used by ptask model",
                                                           "fairbottleneck",
                                                           &simgrid::kernel::lmm::System::validate_solver);

/**************************************/
/*** Resource Creation & Destruction **/
/**************************************/
SIMGRID_REGISTER_HOST_MODEL(
    ptask_L07, "Host model somehow similar to Cas01+CM02+S19 but allowing parallel tasks", []() {
      XBT_CINFO(xbt_cfg, "Switching to the L07 model to handle parallel tasks.");
      xbt_assert(cfg_ptask_solver != "maxmin", "Invalid configuration. Cannot use maxmin solver with parallel tasks.");

      xbt_assert(simgrid::config::is_default("network/model") && simgrid::config::is_default("cpu/model"),
                 "Changing the network or CPU model is not allowed when using the ptasks host model.");

      auto* system    = simgrid::kernel::lmm::System::build(cfg_ptask_solver.get(), true /* selective update */);
      auto host_model = std::make_shared<simgrid::kernel::resource::HostL07Model>("Host_Ptask", system);
      auto* engine    = simgrid::kernel::EngineImpl::get_instance();
      engine->add_model(host_model);
      engine->get_netzone_root()->set_host_model(host_model);
    });

namespace simgrid::kernel::resource {

HostL07Model::HostL07Model(const std::string& name, lmm::System* sys) : HostModel(name)
{
  set_maxmin_system(sys);

  auto net_model = std::make_shared<NetworkL07Model>("Network_Ptask", this, sys);
  auto* engine   = EngineImpl::get_instance();
  engine->add_model(net_model);
  engine->get_netzone_root()->set_network_model(net_model);

  auto cpu_model = std::make_shared<CpuL07Model>("Cpu_Ptask", this, sys);
  engine->add_model(cpu_model);
  engine->get_netzone_root()->set_cpu_pm_model(cpu_model);

  simgrid_disk_models().by_name("S19").init();
}

CpuL07Model::CpuL07Model(const std::string& name, HostL07Model* hmodel, lmm::System* sys)
    : CpuModel(name), hostModel_(hmodel)
{
  set_maxmin_system(sys);
}

CpuL07Model::~CpuL07Model()
{
  set_maxmin_system(nullptr);
}

NetworkL07Model::NetworkL07Model(const std::string& name, HostL07Model* hmodel, lmm::System* sys)
    : NetworkModel(name), hostModel_(hmodel)
{
  set_maxmin_system(sys);
  loopback_.reset(create_link("__loopback__", {simgrid::config::get_value<double>("network/loopback-bw")}));
  loopback_->set_sharing_policy(s4u::Link::SharingPolicy::FATPIPE, {});
  loopback_->set_latency(simgrid::config::get_value<double>("network/loopback-lat"));
  loopback_->get_iface()->seal();
}

NetworkL07Model::~NetworkL07Model()
{
  set_maxmin_system(nullptr);
}

double HostL07Model::next_occurring_event(double now)
{
  double min = HostModel::next_occurring_event_full(now);
  for (Action const& action : *get_started_action_set()) {
    const auto& net_action = static_cast<const L07Action&>(action);
    if (net_action.get_latency() > 0 && (min < 0 || net_action.get_latency() < min)) {
      min = net_action.get_latency();
      XBT_DEBUG("Updating min with %p (start %f): %f", &net_action, net_action.get_start_time(), min);
    }
  }
  XBT_DEBUG("min value: %f", min);

  return min;
}

void HostL07Model::update_actions_state(double /*now*/, double delta)
{
  for (auto it = std::begin(*get_started_action_set()); it != std::end(*get_started_action_set());) {
    auto& action = static_cast<L07Action&>(*it);
    ++it; // increment iterator here since the following calls to action.finish() may invalidate it
    if (action.get_latency() > 0) {
      if (action.get_latency() > delta) {
        action.update_latency(delta, sg_precision_timing);
      } else {
        action.set_latency(0.0);
      }
      if ((action.get_latency() <= 0.0) && (action.is_suspended() == 0)) {
        action.update_bound();
        get_maxmin_system()->update_variable_penalty(action.get_variable(), 1.0);
        action.set_last_update();
      }
    }
    XBT_DEBUG("Action (%p) : remains (%g) updated by %g.", &action, action.get_remains(), action.get_rate() * delta);
    action.update_remains(action.get_rate() * delta);
    action.update_max_duration(delta);

    XBT_DEBUG("Action (%p) : remains (%g).", &action, action.get_remains());

    /* In the next if cascade, the action can be finished either because:
     *  - The amount of remaining work reached 0
     *  - The max duration was reached
     * If it's not done, it may have failed.
     */

    if (((action.get_remains() <= 0) && (action.get_variable()->get_penalty() > 0)) ||
        ((action.get_max_duration() != NO_MAX_DURATION) && (action.get_max_duration() <= 0))) {
      action.finish(Action::State::FINISHED);
      continue;
    }

    /* Need to check that none of the model has failed */
    int i                       = 0;
    const lmm::Constraint* cnst = action.get_variable()->get_constraint(i);
    while (cnst != nullptr) {
      i++;
      if (not cnst->get_id()->is_on()) {
        XBT_DEBUG("Action (%p) Failed!!", &action);
        action.finish(Action::State::FAILED);
        break;
      }
      cnst = action.get_variable()->get_constraint(i);
    }
  }
}

CpuAction* HostL07Model::execute_parallel(const std::vector<s4u::Host*>& host_list, const double* flops_amount,
                                          const double* bytes_amount, double rate)
{
  return new L07Action(this, host_list, flops_amount, bytes_amount, rate);
}

L07Action::L07Action(Model* model, const std::vector<s4u::Host*>& host_list, const double* flops_amount,
                     const double* bytes_amount, double rate)
    : CpuAction(model, 1.0, false)
    , host_list_(host_list)
    , computation_amount_(flops_amount)
    , communication_amount_(bytes_amount)
    , rate_(rate)
{
  size_t link_nb       = 0;
  const size_t host_nb = host_list_.size();
  size_t used_host_nb  = 0; /* Only the hosts with something to compute (>0 flops) are counted) */
  double latency       = 0.0;
  this->set_last_update();

  if (flops_amount != nullptr)
    used_host_nb += std::count_if(flops_amount, flops_amount + host_nb, [](double x) { return x > 0.0; });

  /* Compute the number of affected resources... */
  if (bytes_amount != nullptr) {
    std::unordered_set<const char*> affected_links;

    for (size_t k = 0; k < host_nb * host_nb; k++) {
      if (bytes_amount[k] <= 0)
        continue;

      double lat = 0.0;
      std::vector<StandardLinkImpl*> route;
      host_list_[k / host_nb]->route_to(host_list_[k % host_nb], route, &lat);
      latency = std::max(latency, lat);

      for (auto const* link : route)
        affected_links.insert(link->get_cname());
    }

    link_nb = affected_links.size();
  }

  XBT_DEBUG("Creating a parallel task (%p) with %zu hosts and %zu unique links.", this, host_nb, link_nb);
  latency_ = latency;

  // Allocate more space for constraints (+4) in case users want to mix ptasks and io streams
  set_variable(model->get_maxmin_system()->variable_new(this, 1.0, (rate > 0 ? rate : -1.0), host_nb + link_nb + 4));

  if (latency_ > 0)
    model->get_maxmin_system()->update_variable_penalty(get_variable(), 0.0);

  /* Expand it for the CPUs even if there is nothing to compute, to make sure that it gets expended even if there is no
   * communication either */
  for (size_t i = 0; i < host_nb; i++) {
    model->get_maxmin_system()->expand(host_list[i]->get_cpu()->get_constraint(), get_variable(),
                                       (flops_amount == nullptr ? 0.0 : flops_amount[i]), true);
  }

  if (bytes_amount != nullptr) {
    for (size_t k = 0; k < host_nb * host_nb; k++) {
      if (bytes_amount[k] <= 0.0)
        continue;
      std::vector<StandardLinkImpl*> route;
      host_list_[k / host_nb]->route_to(host_list_[k % host_nb], route, nullptr);

      for (auto const* link : route)
        model->get_maxmin_system()->expand(link->get_constraint(), this->get_variable(), bytes_amount[k]);
    }
  }

  if (link_nb + used_host_nb == 0) {
    this->set_cost(1.0);
    this->set_remains(0.0);
  }
  /* finally calculate the initial bound value */
  update_bound();
}

Action* NetworkL07Model::communicate(s4u::Host* src, s4u::Host* dst, double size, double rate, bool /* streamed */)
{
  std::vector<s4u::Host*> host_list = {src, dst};
  const auto* flops_amount          = new double[2]();
  auto* bytes_amount                = new double[4]();

  bytes_amount[1] = size;

  Action* res = hostModel_->execute_parallel(host_list, flops_amount, bytes_amount, rate);
  static_cast<L07Action*>(res)->free_arrays_ = true;
  return res;
}

CpuImpl* CpuL07Model::create_cpu(s4u::Host* host, const std::vector<double>& speed_per_pstate)
{
  return (new CpuL07(host, speed_per_pstate))->set_model(this);
}

StandardLinkImpl* NetworkL07Model::create_link(const std::string& name, const std::vector<double>& bandwidths)
{
  xbt_assert(bandwidths.size() == 1, "Non WIFI link must have only 1 bandwidth.");
  auto* link = new LinkL07(name, bandwidths[0], get_maxmin_system());
  link->set_model(this);
  return link;
}

StandardLinkImpl* NetworkL07Model::create_wifi_link(const std::string& name, const std::vector<double>& bandwidths)
{
  THROW_UNIMPLEMENTED;
}

/************
 * Resource *
 ************/

CpuAction* CpuL07::execution_start(double size, double user_bound)
{
  std::vector<s4u::Host*> host_list = {get_iface()};
  xbt_assert(user_bound <= 0, "User bound not supported by ptask model");

  auto* flops_amount = new double[host_list.size()]();
  flops_amount[0]    = size;

  CpuAction* res =
      static_cast<CpuL07Model*>(get_model())->hostModel_->execute_parallel(host_list, flops_amount, nullptr, -1);
  static_cast<L07Action*>(res)->free_arrays_ = true;
  return res;
}

CpuAction* CpuL07::sleep(double duration)
{
  auto* action = static_cast<L07Action*>(execution_start(1.0, -1));
  action->set_max_duration(duration);
  action->set_suspend_state(Action::SuspendStates::SLEEPING);
  get_model()->get_maxmin_system()->update_variable_penalty(action->get_variable(), 0.0);

  return action;
}

/** @brief take into account changes of speed (either load or max) */
void CpuL07::on_speed_change()
{
  const lmm::Element* elem = nullptr;

  get_model()->get_maxmin_system()->update_constraint_bound(get_constraint(),
                                                            get_core_count() * speed_.peak * speed_.scale);

  while (const auto* var = get_constraint()->get_variable(&elem)) {
    const auto* action = static_cast<L07Action*>(var->get_id());
    action->update_bound();
  }

  CpuImpl::on_speed_change();
}

LinkL07::LinkL07(const std::string& name, double bandwidth, lmm::System* system) : StandardLinkImpl(name)
{
  this->set_constraint(system->constraint_new(this, bandwidth));
  bandwidth_.peak = bandwidth;
}

void CpuL07::apply_event(profile::Event* triggered, double value)
{
  XBT_DEBUG("Updating cpu %s (%p) with value %g", get_cname(), this, value);
  if (triggered == speed_.event) {
    speed_.scale = value;
    on_speed_change();
    tmgr_trace_event_unref(&speed_.event);

  } else if (triggered == get_state_event()) {
    if (value > 0) {
      if (not is_on()) {
        XBT_VERB("Restart actors on host %s", get_iface()->get_cname());
        get_iface()->turn_on();
      }
    } else
      get_iface()->turn_off();

    unref_state_event();
  } else {
    xbt_die("Unknown event!\n");
  }
}

void LinkL07::apply_event(profile::Event* triggered, double value)
{
  XBT_DEBUG("Updating link %s (%p) with value=%f", get_cname(), this, value);
  if (triggered == bandwidth_.event) {
    set_bandwidth(value);
    tmgr_trace_event_unref(&bandwidth_.event);

  } else if (triggered == latency_.event) {
    set_latency(value);
    tmgr_trace_event_unref(&latency_.event);

  } else if (triggered == get_state_event()) {
    if (value > 0)
      turn_on();
    else
      turn_off();
    unref_state_event();
  } else {
    xbt_die("Unknown event ! \n");
  }
}

void LinkL07::set_bandwidth(double value)
{
  bandwidth_.peak = value;
  StandardLinkImpl::on_bandwidth_change();

  get_model()->get_maxmin_system()->update_constraint_bound(get_constraint(), bandwidth_.peak * bandwidth_.scale);
}

void LinkL07::set_latency(double value)
{
  latency_check(value);
  const lmm::Element* elem = nullptr;

  latency_.peak = value;
  while (const auto* var = get_constraint()->get_variable(&elem)) {
    const auto* action = static_cast<L07Action*>(var->get_id());
    action->update_bound();
  }
}
LinkL07::~LinkL07() = default;

/**********
 * Action *
 **********/

L07Action::~L07Action()
{
  if (free_arrays_) {
    delete[] computation_amount_;
    delete[] communication_amount_;
  }
}

double L07Action::calculate_network_bound() const
{
  double lat_current = 0.0;
  double lat_bound   = std::numeric_limits<double>::max();

  size_t host_count = host_list_.size();

  if (communication_amount_ == nullptr) {
    return lat_bound;
  }

  for (size_t i = 0; i < host_count; i++) {
    for (size_t j = 0; j < host_count; j++) {
      if (communication_amount_[i * host_count + j] > 0) {
        double lat = 0.0;
        std::vector<StandardLinkImpl*> route;
        host_list_.at(i)->route_to(host_list_.at(j), route, &lat);

        lat_current = std::max(lat_current, lat * communication_amount_[i * host_count + j]);
      }
    }
  }
  if (lat_current > 0) {
    lat_bound = NetworkModel::cfg_tcp_gamma / (2.0 * lat_current);
  }
  return lat_bound;
}

double L07Action::calculate_cpu_bound() const
{
  double cpu_bound = std::numeric_limits<double>::max();

  if (computation_amount_ == nullptr) {
    return cpu_bound;
  }

  for (size_t i = 0; i < host_list_.size(); i++) {
    if (computation_amount_[i] > 0) {
      cpu_bound = std::min(cpu_bound, host_list_[i]->get_cpu()->get_speed(1.0) *
                                          host_list_[i]->get_cpu()->get_speed_ratio() / computation_amount_[i]);
    }
  }
  return cpu_bound;
}

void L07Action::update_bound() const
{
  double bound = std::min(calculate_network_bound(), calculate_cpu_bound());

  XBT_DEBUG("action (%p) : bound = %g", this, bound);

  /* latency has been paid (or no latency), we can set the appropriate bound for multicore or network limit */
  if ((bound < std::numeric_limits<double>::max()) && (latency_ <= 0.0)) {
    if (rate_ < 0)
      get_model()->get_maxmin_system()->update_variable_bound(get_variable(), bound);
    else
      get_model()->get_maxmin_system()->update_variable_bound(get_variable(), std::min(rate_, bound));
  }
}

} // namespace simgrid::kernel::resource
