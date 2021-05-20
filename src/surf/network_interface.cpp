/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "network_interface.hpp"
#include "simgrid/s4u/Engine.hpp"
#include "simgrid/sg_config.hpp"
#include "src/kernel/resource/profile/Profile.hpp"
#include "src/surf/surf_interface.hpp"
#include "surf/surf.hpp"

#ifndef NETWORK_INTERFACE_CPP_
#define NETWORK_INTERFACE_CPP_

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(res_network, ker_resource, "Network resources, that fuel communications");

/*********
 * Model *
 *********/

namespace simgrid {
namespace kernel {
namespace resource {

/** @brief Command-line option 'network/TCP-gamma' -- see @ref options_model_network_gamma */
config::Flag<double> NetworkModel::cfg_tcp_gamma(
    "network/TCP-gamma",
    "Size of the biggest TCP window (cat /proc/sys/net/ipv4/tcp_[rw]mem for recv/send window; "
    "Use the last given value, which is the max window size)",
    4194304.0);

/** @brief Command-line option 'network/crosstraffic' -- see @ref options_model_network_crosstraffic */
config::Flag<bool> NetworkModel::cfg_crosstraffic(
    "network/crosstraffic",
    "Activate the interferences between uploads and downloads for fluid max-min models (LV08, CM02)", "yes");

NetworkModel::~NetworkModel() = default;

double NetworkModel::next_occurring_event_full(double now)
{
  double minRes = Model::next_occurring_event_full(now);

  for (Action const& action : *get_started_action_set()) {
    const auto& net_action = static_cast<const NetworkAction&>(action);
    if (net_action.latency_ > 0)
      minRes = (minRes < 0) ? net_action.latency_ : std::min(minRes, net_action.latency_);
  }

  XBT_DEBUG("Min of share resources %f", minRes);

  return minRes;
}

double NetworkModel::get_bandwidth_constraint(double rate, double bound, double size)
{
  return rate < 0 ? bound : std::min(bound, rate);
}

/************
 * Resource *
 ************/

LinkImpl::LinkImpl(const std::string& name) : Resource_T(name), piface_(this)
{
  if (name != "__loopback__")
    xbt_assert(not s4u::Link::by_name_or_null(name), "Link '%s' declared several times in the platform.", name.c_str());

  s4u::Engine::get_instance()->link_register(name, &piface_);
  XBT_DEBUG("Create link '%s'", name.c_str());
}

/** @brief Fire the required callbacks and destroy the object
 *
 * Don't delete directly a Link, call l->destroy() instead.
 */
void LinkImpl::destroy()
{
  s4u::Link::on_destruction(this->piface_);
  delete this;
}

bool LinkImpl::is_used() const
{
  return get_model()->get_maxmin_system()->constraint_used(get_constraint());
}

LinkImpl* LinkImpl::set_sharing_policy(s4u::Link::SharingPolicy policy)
{
  get_constraint()->set_sharing_policy(policy);
  return this;
}
s4u::Link::SharingPolicy LinkImpl::get_sharing_policy() const
{
  return get_constraint()->get_sharing_policy();
}

void LinkImpl::latency_check(double latency) const
{
  static double last_warned_latency = sg_surf_precision;
  if (latency != 0.0 && latency < last_warned_latency) {
    XBT_WARN("Latency for link %s is smaller than surf/precision (%g < %g)."
        " For more accuracy, consider setting \"--cfg=surf/precision:%g\".",
        get_cname(), latency, sg_surf_precision, latency);
    last_warned_latency = latency;
  }
}

void LinkImpl::turn_on()
{
  if (not is_on()) {
    Resource::turn_on();
    s4u::Link::on_state_change(piface_);
  }
}

void LinkImpl::turn_off()
{
  if (is_on()) {
    Resource::turn_off();
    s4u::Link::on_state_change(piface_);

    const kernel::lmm::Element* elem = nullptr;
    double now                       = surf_get_clock();
    while (const auto* var = get_constraint()->get_variable(&elem)) {
      Action* action = var->get_id();
      if (action->get_state() == Action::State::INITED || action->get_state() == Action::State::STARTED) {
        action->set_finish_time(now);
        action->set_state(Action::State::FAILED);
      }
    }
  }
}

void LinkImpl::seal()
{
  if (is_sealed())
    return;

  xbt_assert(this->get_model(), "Cannot seal Link(%s) without setting the Network model first", this->get_cname());
  Resource::seal();
  s4u::Link::on_creation(piface_);
}

void LinkImpl::on_bandwidth_change() const
{
  s4u::Link::on_bandwidth_change(piface_);
}

LinkImpl* LinkImpl::set_bandwidth_profile(profile::Profile* profile)
{
  if (profile) {
    xbt_assert(bandwidth_.event == nullptr, "Cannot set a second bandwidth profile to Link %s", get_cname());
    bandwidth_.event = profile->schedule(&profile::future_evt_set, this);
  }
  return this;
}

LinkImpl* LinkImpl::set_latency_profile(profile::Profile* profile)
{
  if (profile) {
    xbt_assert(latency_.event == nullptr, "Cannot set a second latency profile to Link %s", get_cname());
    latency_.event = profile->schedule(&profile::future_evt_set, this);
  }
  return this;
}

/**********
 * Action *
 **********/

void NetworkAction::set_state(Action::State state)
{
  Action::State previous = get_state();
  if (previous != state) { // Trigger only if the state changed
    Action::set_state(state);
    s4u::Link::on_communication_state_change(*this, previous);
  }
}

/** @brief returns a list of all Links that this action is using */
std::list<LinkImpl*> NetworkAction::get_links() const
{
  std::list<LinkImpl*> retlist;
  int llen = get_variable()->get_number_of_constraint();

  for (int i = 0; i < llen; i++) {
    /* Beware of composite actions: ptasks put links and cpus together */
    if (auto* link = dynamic_cast<LinkImpl*>(get_variable()->get_constraint(i)->get_id()))
      retlist.push_back(link);
  }

  return retlist;
}
} // namespace resource
} // namespace kernel
} // namespace simgrid

#endif /* NETWORK_INTERFACE_CPP_ */
