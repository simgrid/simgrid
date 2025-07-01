/* Copyright (c) 2013-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/kernel/resource/models/network_cm02.hpp"
#include "simgrid/kernel/routing/NetZoneImpl.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/EngineImpl.hpp"
#include "src/kernel/resource/StandardLinkImpl.hpp"
#include "src/kernel/resource/WifiLinkImpl.hpp"
#include "src/kernel/resource/profile/Event.hpp"
#include "src/simgrid/math_utils.h"
#include "src/simgrid/module.hpp"
#include "src/simgrid/sg_config.hpp"
#include "xbt/config.hpp"

#include <algorithm>
#include <numeric>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(res_network);

/***********
 * Options *
 ***********/
static simgrid::config::Flag<std::string> cfg_network_solver("network/solver",
                                                             "Set linear equations solver used by network model",
                                                             "maxmin", &simgrid::kernel::lmm::System::validate_solver);

SIMGRID_REGISTER_NETWORK_MODEL(raw,
                               "Simplest network model with `time = size/bw + lat` and fair sharing. "
                               "No cross traffic, no TCP gamma, no correction factors, just these plain equations.",
                               []() {
                                 auto net_model =
                                     std::make_shared<simgrid::kernel::resource::NetworkCm02Model>("Network_Raw");
                                 auto* engine = simgrid::kernel::EngineImpl::get_instance();
                                 engine->add_model(net_model);
                                 engine->get_netzone_root()->set_network_model(net_model);

                                 simgrid::config::set_default<std::string>("network/latency-factor", "1.0");
                                 simgrid::config::set_default<std::string>("network/bandwidth-factor", "1.0");
                                 simgrid::config::set_default<double>("network/weight-S", 0.0);
                                 simgrid::config::set_default<double>("network/TCP-gamma", 0.0);
                                 simgrid::config::set_default<bool>("network/crosstraffic", false);
                               });

/******************************************************************************/
/* Network model based on optimizations discussed during Pedro Velho's thesis */
/******************************************************************************/
/* @techreport{VELHO:2011:HAL-00646896:1, */
/*      url = {http://hal.inria.fr/hal-00646896/en/}, */
/*      title = {{Flow-level network models: have we reached the limits?}}, */
/*      author = {Velho, Pedro and Schnorr, Lucas and Casanova, Henri and Legrand, Arnaud}, */
/*      type = {Rapport de recherche}, */
/*      institution = {INRIA}, */
/*      number = {RR-7821}, */
/*      year = {2011}, */
/*      month = Nov, */
/*      pdf = {http://hal.inria.fr/hal-00646896/PDF/rr-validity.pdf}, */
/*  } */
SIMGRID_REGISTER_NETWORK_MODEL(
    LV08,
    "Realistic network analytic model (slow-start modeled by multiplying latency by 13.01, bandwidth by .97; "
    "bottleneck sharing uses a payload of S=20537 for evaluating RTT). ",
    []() {
      auto net_model = std::make_shared<simgrid::kernel::resource::NetworkCm02Model>("Network_LegrandVelho");
      auto* engine   = simgrid::kernel::EngineImpl::get_instance();
      engine->add_model(net_model);
      engine->get_netzone_root()->set_network_model(net_model);

      simgrid::config::set_default<std::string>("network/latency-factor", "13.01");
      simgrid::config::set_default<std::string>("network/bandwidth-factor", "0.97");
      simgrid::config::set_default<double>("network/weight-S", 20537);
    });

/****************************************************************************/
/* The older TCP sharing model designed by Loris Marchal and Henri Casanova */
/****************************************************************************/
/* @TechReport{      rr-lip2002-40, */
/*   author        = {Henri Casanova and Loris Marchal}, */
/*   institution   = {LIP}, */
/*   title         = {A Network Model for Simulation of Grid Application}, */
/*   number        = {2002-40}, */
/*   month         = {oct}, */
/*   year          = {2002} */
/* } */
SIMGRID_REGISTER_NETWORK_MODEL(
    CM02,
    "Legacy network analytic model (Very similar to LV08, but without corrective factors. The timings of "
    "small messages are thus poorly modeled).",
    []() {
      simgrid::config::set_default<std::string>("network/latency-factor", "1.0");
      simgrid::config::set_default<std::string>("network/bandwidth-factor", "1.0");
      simgrid::config::set_default<double>("network/weight-S", 0.0);

      auto net_model = std::make_shared<simgrid::kernel::resource::NetworkCm02Model>("Network_CM02");
      auto* engine   = simgrid::kernel::EngineImpl::get_instance();
      engine->add_model(net_model);
      engine->get_netzone_root()->set_network_model(net_model);
    });

/********************************************************************/
/* Model based on LV08 and experimental results of MPI ping-pongs   */
/********************************************************************/
/* @Inproceedings{smpi_ipdps, */
/*  author={Pierre-Nicolas Clauss and Mark Stillwell and Stéphane Genaud and Frédéric Suter and Henri Casanova and
 * Martin Quinson}, */
/*  title={Single Node On-Line Simulation of {MPI} Applications with SMPI}, */
/*  booktitle={25th IEEE International Parallel and Distributed Processing Symposium (IPDPS'11)}, */
/*  address={Anchorage (Alaska) USA}, */
/*  month=may, */
/*  year={2011} */
/*  } */
SIMGRID_REGISTER_NETWORK_MODEL(
    SMPI,
    "Realistic network model specifically tailored for HPC settings (accurate modeling of slow start with "
    "correction factors on three intervals: < 1KiB, < 64 KiB, >= 64 KiB)",
    []() {
      auto net_model = std::make_shared<simgrid::kernel::resource::NetworkCm02Model>("Network_SMPI");
      auto* engine   = simgrid::kernel::EngineImpl::get_instance();
      engine->add_model(net_model);
      engine->get_netzone_root()->set_network_model(net_model);

      simgrid::config::set_default<double>("network/weight-S", 8775);
      simgrid::config::set_default<std::string>("network/bandwidth-factor",
                                                "65472:0.940694;15424:0.697866;9376:0.58729;5776:1.08739;3484:0.77493;"
                                                "1426:0.608902;732:0.341987;257:0.338112;0:0.812084");
      simgrid::config::set_default<std::string>("network/latency-factor",
                                                "65472:11.6436;15424:3.48845;9376:2.59299;5776:2.18796;3484:1.88101;"
                                                "1426:1.61075;732:1.9503;257:1.95341;0:2.01467");
    });

namespace simgrid::kernel::resource {
static simgrid::config::Flag<std::string>
    network_optim_opt("network/optim", "Optimization algorithm to use for network resources. ", "Lazy",

                      std::map<std::string, std::string, std::less<>>({
                          {"Lazy", "Lazy action management (partial invalidation in lmm + heap in action remaining)."},
                          {"Full", "Full update of remaining and variables. Slow but may be useful when debugging."},
                      }),

                      [](std::string const&) {
                        xbt_assert(_sg_cfg_init_status < 2,
                                   "Cannot change the optimization algorithm after the initialization");
                      });

NetworkCm02Model::NetworkCm02Model(const std::string& name) : NetworkModel(name)
{
  bool select = config::get_value<bool>("network/maxmin-selective-update");

  if (network_optim_opt == "Lazy") {
    set_update_algorithm(Model::UpdateAlgo::LAZY);
    xbt_assert(select || config::is_default("network/maxmin-selective-update"),
               "You cannot disable network selective update when using the lazy update mechanism");
    select = true;
  }

  set_maxmin_system(lmm::System::build(cfg_network_solver.get(), select));

  loopback_.reset(create_link("__loopback__", {config::get_value<double>("network/loopback-bw")}));
  loopback_->set_sharing_policy(s4u::Link::SharingPolicy::FATPIPE, {});
  loopback_->set_latency(config::get_value<double>("network/loopback-lat"));
  loopback_->get_iface()->seal();
}

StandardLinkImpl* NetworkCm02Model::create_link(const std::string& name, const std::vector<double>& bandwidths)
{
  xbt_assert(bandwidths.size() == 1, "Non-WIFI links must use only 1 bandwidth.");
  auto* link = new NetworkCm02Link(name, bandwidths[0], get_maxmin_system());
  link->set_model(this);
  return link;
}

StandardLinkImpl* NetworkCm02Model::create_wifi_link(const std::string& name, const std::vector<double>& bandwidths)
{
  auto* link = new WifiLinkImpl(name, bandwidths, get_maxmin_system());
  link->set_model(this);
  return link;
}

void NetworkCm02Model::update_actions_state_lazy(double now, double /*delta*/)
{
  while (not get_action_heap().empty() && double_equals(get_action_heap().top_date(), now, sg_precision_timing)) {
    auto* action = static_cast<NetworkCm02Action*>(get_action_heap().pop());
    XBT_DEBUG("Something happened to action %p", action);

    // if I am wearing a latency hat
    if (action->get_type() == ActionHeap::Type::latency) {
      XBT_DEBUG("Latency paid for action %p. Activating", action);
      get_maxmin_system()->update_variable_penalty(action->get_variable(), action->sharing_penalty_);
      get_action_heap().remove(action);
      action->set_last_update();

      // if I am wearing a max_duration or normal hat
    } else if (action->get_type() == ActionHeap::Type::max_duration || action->get_type() == ActionHeap::Type::normal) {
      // no need to communicate anymore
      // assume that flows that reached max_duration have remaining of 0
      XBT_DEBUG("Action %p finished", action);
      action->finish(Action::State::FINISHED);
      get_action_heap().remove(action);
    }
  }
}

void NetworkCm02Model::update_actions_state_full(double /*now*/, double delta)
{
  for (auto it = std::begin(*get_started_action_set()); it != std::end(*get_started_action_set());) {
    auto& action = static_cast<NetworkCm02Action&>(*it);
    ++it; // increment iterator here since the following calls to action.finish() may invalidate it
    XBT_DEBUG("Something happened to action %p", &action);
    if (action.latency_ > 0) {
      if (action.latency_ > delta) {
        double_update(&action.latency_, delta, sg_precision_timing);
      } else {
        action.latency_ = 0.0;
      }
      if (action.latency_ <= 0.0 && not action.is_suspended())
        get_maxmin_system()->update_variable_penalty(action.get_variable(), action.sharing_penalty_);
    }

    if (not action.get_variable()->get_number_of_constraint()) {
      /* There is actually no link used, hence an infinite bandwidth. This happens often when using models like
       * vivaldi. In such case, just make sure that the action completes immediately.
       */
      action.update_remains(action.get_remains());
    }
    action.update_remains(action.get_rate() * delta);

    if (action.get_max_duration() != NO_MAX_DURATION)
      action.update_max_duration(delta);

    if (((action.get_remains() <= 0) && (action.get_variable()->get_penalty() > 0)) ||
        ((action.get_max_duration() != NO_MAX_DURATION) && (action.get_max_duration() <= 0))) {
      action.finish(Action::State::FINISHED);
    }
  }
}

void NetworkCm02Model::comm_action_expand_constraints(const s4u::Host* src, const s4u::Host* dst,
                                                      const NetworkCm02Action* action,
                                                      const std::vector<StandardLinkImpl*>& route,
                                                      const std::vector<StandardLinkImpl*>& back_route) const
{
  /* expand route links constraints for route and back_route */
  const WifiLinkImpl* src_wifi_link = nullptr;
  const WifiLinkImpl* dst_wifi_link = nullptr;
  if (not route.empty() && route.front()->get_sharing_policy() == s4u::Link::SharingPolicy::WIFI) {
    src_wifi_link = static_cast<WifiLinkImpl*>(route.front());
  }
  if (route.size() > 1 && route.back()->get_sharing_policy() == s4u::Link::SharingPolicy::WIFI) {
    dst_wifi_link = static_cast<WifiLinkImpl*>(route.back());
  }

  /* WI-FI links needs special treatment, do it here */
  if (src_wifi_link != nullptr) {
    if (src_wifi_link->get_host_rate(src) > 0)
      get_maxmin_system()->expand(src_wifi_link->get_constraint(), action->get_variable(),
                                  1.0 / src_wifi_link->get_host_rate(src));
    else {
      get_maxmin_system()->update_variable_penalty(action->get_variable(), 0);
    }
  }

  if (dst_wifi_link != nullptr) {
    if (dst_wifi_link->get_host_rate(dst) > 0)
      get_maxmin_system()->expand(dst_wifi_link->get_constraint(), action->get_variable(),
                                  1.0 / dst_wifi_link->get_host_rate(dst));
    else {
      get_maxmin_system()->update_variable_penalty(action->get_variable(), 0);
    }
  }

  for (auto const* link : route) {
    if (link->get_sharing_policy() != s4u::Link::SharingPolicy::WIFI)
      get_maxmin_system()->expand(link->get_constraint(), action->get_variable(), 1.0);
  }

  if (cfg_crosstraffic) {
    XBT_DEBUG("Crosstraffic active: adding backward flow using 5%% of the available bandwidth");
    if (dst_wifi_link != nullptr)
      get_maxmin_system()->expand(dst_wifi_link->get_constraint(), action->get_variable(),
                                  .05 / dst_wifi_link->get_host_rate(dst));
    if (src_wifi_link != nullptr)
      get_maxmin_system()->expand(src_wifi_link->get_constraint(), action->get_variable(),
                                  .05 / src_wifi_link->get_host_rate(src));

    for (auto const* link : back_route) {
      if (link->get_sharing_policy() != s4u::Link::SharingPolicy::WIFI)
        get_maxmin_system()->expand(link->get_constraint(), action->get_variable(), .05);
    }
  }
}

NetworkCm02Action* NetworkCm02Model::comm_action_create(s4u::Host* src, s4u::Host* dst, double size,
                                                        const std::vector<StandardLinkImpl*>& route, bool failed)
{
  WifiLinkImpl* src_wifi_link = nullptr;
  WifiLinkImpl* dst_wifi_link = nullptr;
  /* many checks related to Wi-Fi links */
  if (not route.empty() && route.front()->get_sharing_policy() == s4u::Link::SharingPolicy::WIFI) {
    src_wifi_link = static_cast<WifiLinkImpl*>(route.front());
    xbt_assert(src_wifi_link->get_host_rate(src) != -1,
               "The route from %s to %s begins with the WIFI link %s, but the host %s does not seem attached to that "
               "WIFI link. Did you call link->set_host_rate()?",
               src->get_cname(), dst->get_cname(), src_wifi_link->get_cname(), src->get_cname());
  }
  if (route.size() > 1 && route.back()->get_sharing_policy() == s4u::Link::SharingPolicy::WIFI) {
    dst_wifi_link = static_cast<WifiLinkImpl*>(route.back());
    xbt_assert(dst_wifi_link->get_host_rate(dst) != -1,
               "The route from %s to %s ends with the WIFI link %s, but the host %s does not seem attached to that "
               "WIFI link. Did you call link->set_host_rate()?",
               src->get_cname(), dst->get_cname(), dst_wifi_link->get_cname(), dst->get_cname());
  }
  if (route.size() > 2)
    for (unsigned i = 1; i < route.size() - 1; i++)
      xbt_assert(route[i]->get_sharing_policy() != s4u::Link::SharingPolicy::WIFI,
                 "Link '%s' is a WIFI link. It can only be at the beginning or the end of the route from '%s' to '%s', "
                 "not in between (it is at position %u out of %zu). "
                 "Did you declare an access_point in your WIFI zones?",
                 route[i]->get_cname(), src->get_cname(), dst->get_cname(), i + 1, route.size());

  for (auto const* link : route) {
    if (link->get_sharing_policy() == s4u::Link::SharingPolicy::WIFI) {
      xbt_assert(link == src_wifi_link || link == dst_wifi_link,
                 "Wifi links can only occur at the beginning of the route (meaning that it's attached to the src) or "
                 "at its end (meaning that it's attached to the dst");
    }
  }

  /* create action and do some initializations */
  NetworkCm02Action* action;
  if (src_wifi_link == nullptr && dst_wifi_link == nullptr)
    action = new NetworkCm02Action(this, *src, *dst, size, failed);
  else
    action = new WifiLinkAction(this, *src, *dst, size, failed, src_wifi_link, dst_wifi_link);

  if (is_update_lazy()) {
    action->set_last_update();
  }

  return action;
}

bool NetworkCm02Model::comm_get_route_info(const s4u::Host* src, const s4u::Host* dst, double& latency,
                                           std::vector<StandardLinkImpl*>& route,
                                           std::vector<StandardLinkImpl*>& back_route,
                                           std::unordered_set<kernel::routing::NetZoneImpl*>& netzones) const
{
  kernel::routing::NetZoneImpl::get_global_route_with_netzones(src->get_netpoint(), dst->get_netpoint(), route,
                                                               &latency, netzones);

  xbt_assert(not route.empty() || latency > 0,
             "You're trying to send data from %s to %s but there is no connecting path between these two hosts.",
             src->get_cname(), dst->get_cname());

  bool failed = std::any_of(route.begin(), route.end(), [](const StandardLinkImpl* link) { return not link->is_on(); });

  if (not failed && cfg_crosstraffic) {
    dst->route_to(src, back_route, nullptr);
    failed = std::any_of(back_route.begin(), back_route.end(),
                         [](const StandardLinkImpl* link) { return not link->is_on(); });
  }
  return failed;
}

void NetworkCm02Model::comm_action_set_bounds(const s4u::Host* src, const s4u::Host* dst, double size,
                                              NetworkCm02Action* action, const std::vector<StandardLinkImpl*>& route,
                                              const std::unordered_set<kernel::routing::NetZoneImpl*>& netzones,
                                              double rate) const
{
  std::vector<s4u::Link*> s4u_route;
  std::unordered_set<s4u::NetZone*> s4u_netzones;

  /* transform data to user structures if necessary */
  if (has_network_factor_cb()) {
    std::for_each(route.begin(), route.end(),
                  [&s4u_route](StandardLinkImpl* l) { s4u_route.push_back(l->get_iface()); });
    std::for_each(netzones.begin(), netzones.end(),
                  [&s4u_netzones](kernel::routing::NetZoneImpl* n) { s4u_netzones.insert(n->get_iface()); });
  }

  double bw_factor = get_bandwidth_factor(size, src, dst, s4u_route, s4u_netzones);
  xbt_assert(bw_factor != 0, "Invalid param for comm %s -> %s. Bandwidth factor cannot be 0", src->get_cname(),
             dst->get_cname());
  action->set_rate_factor(bw_factor);

  /* get mininum bandwidth among links in the route and multiply by correct factor
   * ignore wi-fi links, they're not considered for bw_factors */
  double bandwidth_bound = -1.0;
  for (const auto* l : route) {
    if (l->get_sharing_policy() == s4u::Link::SharingPolicy::WIFI)
      continue;
    if (bandwidth_bound == -1.0 || l->get_bandwidth() < bandwidth_bound)
      bandwidth_bound = l->get_bandwidth();
  }

  /* increase rate given by user considering the factor, since the actual rate will be
   * modified by it */
  rate = rate / bw_factor;
  /* the bandwidth is determined by the minimum between flow and user's defined rate */
  if (rate >= 0 && rate < bandwidth_bound)
    bandwidth_bound = rate;
  action->set_user_bound(bandwidth_bound);

  action->lat_current_ = action->latency_;
  action->latency_ *= get_latency_factor(size, src, dst, s4u_route, s4u_netzones);
}

void NetworkCm02Model::comm_action_set_variable(NetworkCm02Action* action, const std::vector<StandardLinkImpl*>& route,
                                                const std::vector<StandardLinkImpl*>& back_route, bool streamed)
{
  size_t constraints_per_variable = route.size();
  constraints_per_variable += back_route.size();
  if (streamed) {
    // setting the number of variable for a communication action involved in a I/O streaming operation
    // requires to reserve some extra space for the constraints related to the source disk (global and read
    // bandwidth) and destination disk (global and write bandwidth). We thus add 4 constraints.
    constraints_per_variable += 4;
  }

  if (action->latency_ > 0) {
    action->set_variable(get_maxmin_system()->variable_new(action, 0.0, -1.0, constraints_per_variable));
    if (is_update_lazy()) {
      // add to the heap the event when the latency is paid
      double date = action->latency_ + action->get_last_update();

      ActionHeap::Type type = route.empty() ? ActionHeap::Type::normal : ActionHeap::Type::latency;

      XBT_DEBUG("Added action (%p) one latency event at date %f", action, date);
      get_action_heap().insert(action, date, type);
    }
  } else
    action->set_variable(get_maxmin_system()->variable_new(action, 1.0, -1.0, constraints_per_variable));

  /* after setting the variable, update the bounds depending on user configuration */
  if (action->get_user_bound() < 0) {
    get_maxmin_system()->update_variable_bound(
        action->get_variable(),
        (action->lat_current_ > 0 && cfg_tcp_gamma > 0) ? cfg_tcp_gamma / (2.0 * action->lat_current_) : -1.0);
  } else {
    get_maxmin_system()->update_variable_bound(
        action->get_variable(), (action->lat_current_ > 0 && cfg_tcp_gamma > 0)
                                    ? std::min(action->get_user_bound(), cfg_tcp_gamma / (2.0 * action->lat_current_))
                                    : action->get_user_bound());
  }
}

Action* NetworkCm02Model::communicate(s4u::Host* src, s4u::Host* dst, double size, double rate, bool streamed)
{
  double latency = 0.0;
  std::vector<StandardLinkImpl*> back_route;
  std::vector<StandardLinkImpl*> route;
  std::unordered_set<kernel::routing::NetZoneImpl*> netzones;

  XBT_IN("(%s,%s,%g,%g)", src->get_cname(), dst->get_cname(), size, rate);

  bool failed = comm_get_route_info(src, dst, latency, route, back_route, netzones);

  NetworkCm02Action* action = comm_action_create(src, dst, size, route, failed);
  action->sharing_penalty_  = latency;
  action->latency_          = latency;

  if (cfg_weight_S_parameter > 0) {
    action->sharing_penalty_ = std::accumulate(route.begin(), route.end(), action->sharing_penalty_,
                                               [](double total, StandardLinkImpl const* link) {
                                                 return total + cfg_weight_S_parameter / link->get_bandwidth();
                                               });
  }

  /* setting bandwidth and latency bounds considering route and configured bw/lat factors */
  comm_action_set_bounds(src, dst, size, action, route, netzones, rate);

  /* creating the maxmin variable associated to this action */
  comm_action_set_variable(action, route, back_route, streamed);

  /* expand maxmin system to consider this communication in bw constraint for each link in route and back_route */
  comm_action_expand_constraints(src, dst, action, route, back_route);
  XBT_OUT();

  return action;
}

/************
 * Resource *
 ************/
NetworkCm02Link::NetworkCm02Link(const std::string& name, double bandwidth, kernel::lmm::System* system)
    : StandardLinkImpl(name)
{
  bandwidth_.scale = 1.0;
  bandwidth_.peak  = bandwidth;
  this->set_constraint(system->constraint_new(this, bandwidth));
}

void NetworkCm02Link::apply_event(kernel::profile::Event* triggered, double value)
{
  /* Find out which of my iterators was triggered, and react accordingly */
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
    xbt_die("Unknown event!\n");
  }

  XBT_DEBUG("There was a resource state event, need to update actions related to the constraint (%p)",
            get_constraint());
}

void NetworkCm02Link::set_bandwidth(double value)
{
  double old_peak = bandwidth_.peak;
  bandwidth_.peak = value;

  get_model()->get_maxmin_system()->update_constraint_bound(get_constraint(), (bandwidth_.peak * bandwidth_.scale));

  StandardLinkImpl::on_bandwidth_change();

  if (NetworkModel::cfg_weight_S_parameter > 0) {
    double delta = NetworkModel::cfg_weight_S_parameter / (bandwidth_.peak * bandwidth_.scale) -
                   NetworkModel::cfg_weight_S_parameter / (old_peak * bandwidth_.scale);

    const kernel::lmm::Element* elem     = nullptr;
    const kernel::lmm::Element* nextelem = nullptr;
    size_t numelem                       = 0;
    while (const auto* var = get_constraint()->get_variable_safe(&elem, &nextelem, &numelem)) {
      auto* action = static_cast<NetworkCm02Action*>(var->get_id());
      action->sharing_penalty_ += delta;
      if (not action->is_suspended())
        get_model()->get_maxmin_system()->update_variable_penalty(action->get_variable(), action->sharing_penalty_);
    }
  }
}

void NetworkCm02Link::set_latency(double value)
{
  latency_check(value);

  double delta                         = value - latency_.peak;
  const kernel::lmm::Element* elem     = nullptr;
  const kernel::lmm::Element* nextelem = nullptr;
  size_t numelem                       = 0;

  latency_.scale = 1.0;
  latency_.peak  = value;

  while (const auto* var = get_constraint()->get_variable_safe(&elem, &nextelem, &numelem)) {
    auto* action = static_cast<NetworkCm02Action*>(var->get_id());
    action->lat_current_ += delta;
    action->sharing_penalty_ += delta;
    if (action->get_user_bound() < 0 && NetworkModel::cfg_tcp_gamma > 0)
      get_model()->get_maxmin_system()->update_variable_bound(action->get_variable(), NetworkModel::cfg_tcp_gamma /
                                                                                          (2.0 * action->lat_current_));
    else if (NetworkModel::cfg_tcp_gamma > 0) {
      get_model()->get_maxmin_system()->update_variable_bound(
          action->get_variable(),
          std::min(action->get_user_bound(), NetworkModel::cfg_tcp_gamma / (2.0 * action->lat_current_)));
    }
    if (NetworkModel::cfg_tcp_gamma == 0 ||
        action->get_user_bound() < NetworkModel::cfg_tcp_gamma / (2.0 * action->lat_current_)) {
      XBT_DEBUG("Flow is limited BYBANDWIDTH");
    } else {
      XBT_DEBUG("Flow is limited BYLATENCY, latency of flow is %f", action->lat_current_);
    }

    if (not action->is_suspended())
      get_model()->get_maxmin_system()->update_variable_penalty(action->get_variable(), action->sharing_penalty_);
  }
}

/**********
 * Action *
 **********/

void NetworkCm02Action::update_remains_lazy(double now)
{
  if (not is_running())
    return;

  double delta = now - get_last_update();

  if (get_remains_no_update() > 0) {
    XBT_DEBUG("Updating action(%p): remains was %f, last_update was: %f", this, get_remains_no_update(),
              get_last_update());
    update_remains(get_last_value() * delta);

    XBT_DEBUG("Updating action(%p): remains is now %f", this, get_remains_no_update());
  }

  update_max_duration(delta);

  if ((get_remains_no_update() <= 0 && (get_variable()->get_penalty() > 0)) ||
      ((get_max_duration() != NO_MAX_DURATION) && (get_max_duration() <= 0))) {
    finish(Action::State::FINISHED);
    get_model()->get_action_heap().remove(this);
  }

  set_last_update();
  set_last_value(get_rate());
}

} // namespace simgrid::kernel::resource
