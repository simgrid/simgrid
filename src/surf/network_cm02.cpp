/* Copyright (c) 2013-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/network_cm02.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/sg_config.hpp"
#include "src/kernel/resource/profile/Event.hpp"
#include "src/surf/network_wifi.hpp"
#include "src/surf/surf_interface.hpp"
#include "surf/surf.hpp"

#include <algorithm>
#include <numeric>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(surf_network);

double sg_latency_factor = 1.0; /* default value; can be set by model or from command line */
double sg_bandwidth_factor = 1.0;       /* default value; can be set by model or from command line */
double sg_weight_S_parameter = 0.0;     /* default value; can be set by model or from command line */

/************************************************************************/
/* New model based on optimizations discussed during Pedro Velho's thesis*/
/************************************************************************/
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
void surf_network_model_init_LegrandVelho()
{
  xbt_assert(surf_network_model == nullptr, "Cannot set the network model twice");

  surf_network_model = new simgrid::kernel::resource::NetworkCm02Model();

  simgrid::config::set_default<double>("network/latency-factor", 13.01);
  simgrid::config::set_default<double>("network/bandwidth-factor", 0.97);
  simgrid::config::set_default<double>("network/weight-S", 20537);
}

/***************************************************************************/
/* The nice TCP sharing model designed by Loris Marchal and Henri Casanova */
/***************************************************************************/
/* @TechReport{      rr-lip2002-40, */
/*   author        = {Henri Casanova and Loris Marchal}, */
/*   institution   = {LIP}, */
/*   title         = {A Network Model for Simulation of Grid Application}, */
/*   number        = {2002-40}, */
/*   month         = {oct}, */
/*   year          = {2002} */
/* } */
void surf_network_model_init_CM02()
{
  xbt_assert(surf_network_model == nullptr, "Cannot set the network model twice");

  simgrid::config::set_default<double>("network/latency-factor", 1.0);
  simgrid::config::set_default<double>("network/bandwidth-factor", 1.0);
  simgrid::config::set_default<double>("network/weight-S", 0.0);

  surf_network_model = new simgrid::kernel::resource::NetworkCm02Model();
}

namespace simgrid {
namespace kernel {
namespace resource {

NetworkCm02Model::NetworkCm02Model(kernel::lmm::System* (*make_new_lmm_system)(bool))
    : NetworkModel(simgrid::config::get_value<std::string>("network/optim") == "Full" ? Model::UpdateAlgo::FULL
                                                                                      : Model::UpdateAlgo::LAZY)
{
  all_existing_models.push_back(this);

  std::string optim = simgrid::config::get_value<std::string>("network/optim");
  bool select       = simgrid::config::get_value<bool>("network/maxmin-selective-update");

  if (optim == "Lazy") {
    xbt_assert(select || simgrid::config::is_default("network/maxmin-selective-update"),
               "You cannot disable network selective update when using the lazy update mechanism");
    select = true;
  }

  set_maxmin_system(make_new_lmm_system(select));
  loopback_ = NetworkCm02Model::create_link("__loopback__", std::vector<double>(1, 498000000), 0.000015,
                                            s4u::Link::SharingPolicy::FATPIPE);
}

LinkImpl* NetworkCm02Model::create_link(const std::string& name, const std::vector<double>& bandwidths, double latency,
                                        s4u::Link::SharingPolicy policy)
{
  if (policy == s4u::Link::SharingPolicy::WIFI)
    return new NetworkWifiLink(this, name, bandwidths, policy, get_maxmin_system());

  xbt_assert(bandwidths.size() == 1, "Non-WIFI links must use only 1 bandwidth.");
  return new NetworkCm02Link(this, name, bandwidths[0], latency, policy, get_maxmin_system());
}

void NetworkCm02Model::update_actions_state_lazy(double now, double /*delta*/)
{
  while (not get_action_heap().empty() && double_equals(get_action_heap().top_date(), now, sg_surf_precision)) {

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
    NetworkCm02Action& action = static_cast<NetworkCm02Action&>(*it);
    ++it; // increment iterator here since the following calls to action.finish() may invalidate it
    XBT_DEBUG("Something happened to action %p", &action);
    double deltap = delta;
    if (action.latency_ > 0) {
      if (action.latency_ > deltap) {
        double_update(&action.latency_, deltap, sg_surf_precision);
        deltap = 0.0;
      } else {
        double_update(&deltap, action.latency_, sg_surf_precision);
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
    action.update_remains(action.get_variable()->get_value() * delta);

    if (action.get_max_duration() != NO_MAX_DURATION)
      action.update_max_duration(delta);

    if (((action.get_remains() <= 0) && (action.get_variable()->get_penalty() > 0)) ||
        ((action.get_max_duration() != NO_MAX_DURATION) && (action.get_max_duration() <= 0))) {
      action.finish(Action::State::FINISHED);
    }
  }
}

Action* NetworkCm02Model::communicate(s4u::Host* src, s4u::Host* dst, double size, double rate)
{
  double latency = 0.0;
  std::vector<LinkImpl*> back_route;
  std::vector<LinkImpl*> route;

  XBT_IN("(%s,%s,%g,%g)", src->get_cname(), dst->get_cname(), size, rate);

  src->route_to(dst, route, &latency);
  xbt_assert(not route.empty() || latency > 0,
             "You're trying to send data from %s to %s but there is no connecting path between these two hosts.",
             src->get_cname(), dst->get_cname());

  bool failed = std::any_of(route.begin(), route.end(), [](const LinkImpl* link) { return not link->is_on(); });

  if (cfg_crosstraffic) {
    dst->route_to(src, back_route, nullptr);
    if (not failed)
      failed =
          std::any_of(back_route.begin(), back_route.end(), [](const LinkImpl* link) { return not link->is_on(); });
  }

  auto* action              = new NetworkCm02Action(this, size, failed);
  action->sharing_penalty_  = latency;
  action->latency_ = latency;
  action->rate_ = rate;

  if (get_update_algorithm() == Model::UpdateAlgo::LAZY) {
    action->set_last_update();
  }

  if (sg_weight_S_parameter > 0) {
    action->sharing_penalty_ =
        std::accumulate(route.begin(), route.end(), action->sharing_penalty_, [](double total, LinkImpl* const& link) {
          return total + sg_weight_S_parameter / link->get_bandwidth();
        });
  }

  double bandwidth_bound = route.empty() ? -1.0 : get_bandwidth_factor(size) * route.front()->get_bandwidth();

  for (auto const& link : route)
    bandwidth_bound = std::min(bandwidth_bound, get_bandwidth_factor(size) * link->get_bandwidth());

  action->lat_current_ = action->latency_;
  action->latency_ *= get_latency_factor(size);
  action->rate_ = get_bandwidth_constraint(action->rate_, bandwidth_bound, size);

  size_t constraints_per_variable = route.size();
  constraints_per_variable += back_route.size();

  if (action->latency_ > 0) {
    action->set_variable(get_maxmin_system()->variable_new(action, 0.0, -1.0, constraints_per_variable));
    if (get_update_algorithm() == Model::UpdateAlgo::LAZY) {
      // add to the heap the event when the latency is payed
      double date = action->latency_ + action->get_last_update();

      ActionHeap::Type type = route.empty() ? ActionHeap::Type::normal : ActionHeap::Type::latency;

      XBT_DEBUG("Added action (%p) one latency event at date %f", action, date);
      get_action_heap().insert(action, date, type);
    }
  } else
    action->set_variable(get_maxmin_system()->variable_new(action, 1.0, -1.0, constraints_per_variable));

  if (action->rate_ < 0) {
    get_maxmin_system()->update_variable_bound(
        action->get_variable(), (action->lat_current_ > 0) ? cfg_tcp_gamma / (2.0 * action->lat_current_) : -1.0);
  } else {
    get_maxmin_system()->update_variable_bound(
        action->get_variable(), (action->lat_current_ > 0)
                                    ? std::min(action->rate_, cfg_tcp_gamma / (2.0 * action->lat_current_))
                                    : action->rate_);
  }

  for (auto const& link : route) {
    // Handle WIFI links
    if (link->get_sharing_policy() == s4u::Link::SharingPolicy::WIFI) {
      NetworkWifiLink* wifi_link = static_cast<NetworkWifiLink*>(link);

      double src_rate = wifi_link->get_host_rate(src);
      double dst_rate = wifi_link->get_host_rate(dst);
      xbt_assert(
          !(src_rate == -1 && dst_rate == -1),
          "Some Stations are not associated to any Access Point. Make sure to call set_host_rate on all Stations.");
      if (src_rate != -1)
        get_maxmin_system()->expand(link->get_constraint(), action->get_variable(), 1.0 / src_rate);
      else
        get_maxmin_system()->expand(link->get_constraint(), action->get_variable(), 1.0 / dst_rate);

    } else {
      get_maxmin_system()->expand(link->get_constraint(), action->get_variable(), 1.0);
    }
  }

  if (cfg_crosstraffic) {
    XBT_DEBUG("Crosstraffic active: adding backward flow using 5%% of the available bandwidth");
    bool wifi_dst_assigned = false; // Used by wifi crosstraffic
    for (auto const& link : back_route) {
      if (link->get_sharing_policy() == s4u::Link::SharingPolicy::WIFI) {
        NetworkWifiLink* wifi_link = static_cast<NetworkWifiLink*>(link);
        /**
         * For wifi links we should add 0.05/rate.
         * However since we are using the "back_route" we should encounter in
         * the first place the dst wifi link.
         */
        if (!wifi_dst_assigned && (wifi_link->get_host_rate(dst) != -1)) {
          get_maxmin_system()->expand(link->get_constraint(), action->get_variable(),
                                      .05 / wifi_link->get_host_rate(dst));
          wifi_dst_assigned = true;
        } else {
          get_maxmin_system()->expand(link->get_constraint(), action->get_variable(),
                                      .05 / wifi_link->get_host_rate(src));
        }
      } else {
        get_maxmin_system()->expand(link->get_constraint(), action->get_variable(), .05);
      }
    }

    // Change concurrency_share here, if you want that cross-traffic is included in the SURF concurrency
    // (You would also have to change simgrid::kernel::lmm::Element::get_concurrency())
    // action->getVariable()->set_concurrency_share(2)
  }
  XBT_OUT();

  simgrid::s4u::Link::on_communicate(*action, src, dst);
  return action;
}

/************
 * Resource *
 ************/
NetworkCm02Link::NetworkCm02Link(NetworkCm02Model* model, const std::string& name, double bandwidth, double latency,
                                 s4u::Link::SharingPolicy policy, kernel::lmm::System* system)
    : LinkImpl(model, name, system->constraint_new(this, sg_bandwidth_factor * bandwidth))
{
  bandwidth_.scale = 1.0;
  bandwidth_.peak  = bandwidth;

  latency_.scale = 1.0;
  latency_.peak  = latency;

  if (policy == s4u::Link::SharingPolicy::FATPIPE)
    get_constraint()->unshare();

  simgrid::s4u::Link::on_creation(this->piface_);
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

  } else if (triggered == state_event_) {
    if (value > 0)
      turn_on();
    else
      turn_off();
    tmgr_trace_event_unref(&state_event_);
  } else {
    xbt_die("Unknown event!\n");
  }

  XBT_DEBUG("There was a resource state event, need to update actions related to the constraint (%p)",
            get_constraint());
}

void NetworkCm02Link::set_bandwidth(double value)
{
  bandwidth_.peak = value;

  get_model()->get_maxmin_system()->update_constraint_bound(get_constraint(),
                                                            sg_bandwidth_factor * (bandwidth_.peak * bandwidth_.scale));

  LinkImpl::on_bandwidth_change();

  if (sg_weight_S_parameter > 0) {
    double delta = sg_weight_S_parameter / value - sg_weight_S_parameter / (bandwidth_.peak * bandwidth_.scale);

    kernel::lmm::Variable* var;
    const kernel::lmm::Element* elem     = nullptr;
    const kernel::lmm::Element* nextelem = nullptr;
    int numelem                  = 0;
    while ((var = get_constraint()->get_variable_safe(&elem, &nextelem, &numelem))) {
      auto* action = static_cast<NetworkCm02Action*>(var->get_id());
      action->sharing_penalty_ += delta;
      if (not action->is_suspended())
        get_model()->get_maxmin_system()->update_variable_penalty(action->get_variable(), action->sharing_penalty_);
    }
  }
}

void NetworkCm02Link::set_latency(double value)
{
  double delta                 = value - latency_.peak;
  kernel::lmm::Variable* var   = nullptr;
  const kernel::lmm::Element* elem     = nullptr;
  const kernel::lmm::Element* nextelem = nullptr;
  int numelem                  = 0;

  latency_.peak = value;

  while ((var = get_constraint()->get_variable_safe(&elem, &nextelem, &numelem))) {
    auto* action = static_cast<NetworkCm02Action*>(var->get_id());
    action->lat_current_ += delta;
    action->sharing_penalty_ += delta;
    if (action->rate_ < 0)
      get_model()->get_maxmin_system()->update_variable_bound(action->get_variable(), NetworkModel::cfg_tcp_gamma /
                                                                                          (2.0 * action->lat_current_));
    else {
      get_model()->get_maxmin_system()->update_variable_bound(
          action->get_variable(), std::min(action->rate_, NetworkModel::cfg_tcp_gamma / (2.0 * action->lat_current_)));

      if (action->rate_ < NetworkModel::cfg_tcp_gamma / (2.0 * action->lat_current_)) {
        XBT_INFO("Flow is limited BYBANDWIDTH");
      } else {
        XBT_INFO("Flow is limited BYLATENCY, latency of flow is %f", action->lat_current_);
      }
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
  set_last_value(get_variable()->get_value());
}

} // namespace resource
} // namespace kernel
} // namespace simgrid
