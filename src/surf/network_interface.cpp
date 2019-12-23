/* Copyright (c) 2013-2019. The SimGrid Team. All rights reserved.          */

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

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(surf_network, surf, "Logging specific to the SURF network module");

/*********
 * Model *
 *********/

simgrid::kernel::resource::NetworkModel* surf_network_model = nullptr;

namespace simgrid {
namespace kernel {
namespace resource {

/** @brief Command-line option 'network/TCP-gamma' -- see @ref options_model_network_gamma */
simgrid::config::Flag<double> NetworkModel::cfg_tcp_gamma(
    "network/TCP-gamma", {"network/TCP_gamma"},
    "Size of the biggest TCP window (cat /proc/sys/net/ipv4/tcp_[rw]mem for recv/send window; "
    "Use the last given value, which is the max window size)",
    4194304.0);

/** @brief Command-line option 'network/crosstraffic' -- see @ref options_model_network_crosstraffic */
simgrid::config::Flag<bool> NetworkModel::cfg_crosstraffic(
    "network/crosstraffic",
    "Activate the interferences between uploads and downloads for fluid max-min models (LV08, CM02)", "yes");

NetworkModel::~NetworkModel() = default;

double NetworkModel::get_latency_factor(double /*size*/)
{
  return sg_latency_factor;
}

double NetworkModel::get_bandwidth_factor(double /*size*/)
{
  return sg_bandwidth_factor;
}

double NetworkModel::get_bandwidth_constraint(double rate, double /*bound*/, double /*size*/)
{
  return rate;
}

double NetworkModel::next_occurring_event_full(double now)
{
  double minRes = Model::next_occurring_event_full(now);

  for (Action const& action : *get_started_action_set()) {
    const NetworkAction& net_action = static_cast<const NetworkAction&>(action);
    if (net_action.latency_ > 0)
      minRes = (minRes < 0) ? net_action.latency_ : std::min(minRes, net_action.latency_);
  }

  XBT_DEBUG("Min of share resources %f", minRes);

  return minRes;
}

/************
 * Resource *
 ************/

LinkImpl::LinkImpl(NetworkModel* model, const std::string& name, lmm::Constraint* constraint)
    : Resource(model, name, constraint), piface_(this)
{
  if (name != "__loopback__")
    xbt_assert(not s4u::Link::by_name_or_null(name), "Link '%s' declared several times in the platform.", name.c_str());

  latency_.scale   = 1;
  bandwidth_.scale = 1;

  s4u::Engine::get_instance()->link_register(name, &piface_);
  XBT_DEBUG("Create link '%s'", name.c_str());
}

/** @brief use destroy() instead of this destructor */
LinkImpl::~LinkImpl()
{
  xbt_assert(currently_destroying_, "Don't delete Links directly. Call destroy() instead.");
}
/** @brief Fire the required callbacks and destroy the object
 *
 * Don't delete directly a Link, call l->destroy() instead.
 */
void LinkImpl::destroy()
{
  if (not currently_destroying_) {
    currently_destroying_ = true;
    s4u::Link::on_destruction(this->piface_);
    delete this;
  }
}

bool LinkImpl::is_used()
{
  return get_model()->get_maxmin_system()->constraint_used(get_constraint());
}

double LinkImpl::get_latency()
{
  return latency_.peak * latency_.scale;
}

double LinkImpl::get_bandwidth()
{
  return bandwidth_.peak * bandwidth_.scale;
}

s4u::Link::SharingPolicy LinkImpl::get_sharing_policy()
{
  return get_constraint()->get_sharing_policy();
}

void LinkImpl::turn_on()
{
  if (not is_on()) {
    Resource::turn_on();
    s4u::Link::on_state_change(this->piface_);
  }
}

void LinkImpl::turn_off()
{
  if (is_on()) {
    Resource::turn_off();
    s4u::Link::on_state_change(this->piface_);

    const kernel::lmm::Variable* var;
    const kernel::lmm::Element* elem = nullptr;
    double now                       = surf_get_clock();
    while ((var = get_constraint()->get_variable(&elem))) {
      Action* action = static_cast<Action*>(var->get_id());
      if (action->get_state() == Action::State::INITED || action->get_state() == Action::State::STARTED) {
        action->set_finish_time(now);
        action->set_state(Action::State::FAILED);
      }
    }
  }
}

void LinkImpl::on_bandwidth_change()
{
  s4u::Link::on_bandwidth_change(this->piface_);
}

void LinkImpl::set_bandwidth_profile(profile::Profile* profile)
{
  xbt_assert(bandwidth_.event == nullptr, "Cannot set a second bandwidth profile to Link %s", get_cname());
  bandwidth_.event = profile->schedule(&profile::future_evt_set, this);
}

void LinkImpl::set_latency_profile(profile::Profile* profile)
{
  xbt_assert(latency_.event == nullptr, "Cannot set a second latency profile to Link %s", get_cname());
  latency_.event = profile->schedule(&profile::future_evt_set, this);
}

/**********
 * Action *
 **********/

void NetworkAction::set_state(Action::State state)
{
  Action::State previous = get_state();
  Action::set_state(state);
  if (previous != state) // Trigger only if the state changed
    s4u::Link::on_communication_state_change(*this, previous);
}

/** @brief returns a list of all Links that this action is using */
std::list<LinkImpl*> NetworkAction::links() const
{
  std::list<LinkImpl*> retlist;
  int llen = get_variable()->get_number_of_constraint();

  for (int i = 0; i < llen; i++) {
    /* Beware of composite actions: ptasks put links and cpus together */
    // extra pb: we cannot dynamic_cast from void*...
    Resource* resource = get_variable()->get_constraint(i)->get_id();
    LinkImpl* link     = dynamic_cast<LinkImpl*>(resource);
    if (link != nullptr)
      retlist.push_back(link);
  }

  return retlist;
}
}
} // namespace kernel
}

#endif /* NETWORK_INTERFACE_CPP_ */
