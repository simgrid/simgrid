/* Copyright (c) 2013-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/Engine.hpp>

#include "src/kernel/resource/LinkImpl.hpp"
#include "src/kernel/resource/StandardLinkImpl.hpp"
#include <numeric>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(res_network);

/*********
 * Model *
 *********/

namespace simgrid::kernel::resource {

StandardLinkImpl::StandardLinkImpl(const std::string& name, s4u::Link::SharingPolicy sharing_policy,
                                   routing::NetZoneImpl* englobing_zone)
    : LinkImpl(name, sharing_policy, englobing_zone), piface_(this)
{
  if (name != "__loopback__")
    xbt_assert(not s4u::Link::by_name_or_null(name), "Link '%s' declared several times in the platform.", name.c_str());

  XBT_DEBUG("Create link '%s'", name.c_str());
}

void StandardLinkImpl::Deleter::operator()(resource::StandardLinkImpl* link) const
{
  link->destroy();
}

/** @brief Fire the required callbacks and destroy the object
 *
 * Don't delete directly a Link, call l->destroy() instead.
 */
void StandardLinkImpl::destroy()
{
  s4u::Link::on_destruction(piface_);
  piface_.on_this_destruction(piface_);
  delete this;
}

constexpr kernel::lmm::Constraint::SharingPolicy to_maxmin_policy(s4u::Link::SharingPolicy policy)
{
  switch (policy) {
    case s4u::Link::SharingPolicy::WIFI:
      return kernel::lmm::Constraint::SharingPolicy::WIFI;
    case s4u::Link::SharingPolicy::NONLINEAR:
      return kernel::lmm::Constraint::SharingPolicy::NONLINEAR;
    case s4u::Link::SharingPolicy::FATPIPE:
      return kernel::lmm::Constraint::SharingPolicy::FATPIPE;
    default:
      return kernel::lmm::Constraint::SharingPolicy::SHARED;
  }
}

void StandardLinkImpl::set_sharing_policy(s4u::Link::SharingPolicy policy, const s4u::NonLinearResourceCb& cb)
{
  if (auto constraint = get_constraint(); constraint != nullptr)
    constraint->set_sharing_policy(to_maxmin_policy(policy), cb);
  LinkImpl::set_sharing_policy(policy, cb);
}

void StandardLinkImpl::latency_check(double latency) const
{
  static double last_warned_latency = sg_precision_timing;
  if (latency != 0.0 && latency < last_warned_latency) {
    XBT_WARN("Latency for link %s is smaller than precision/timing (%g < %g)."
             " For more accuracy, consider setting \"--cfg=precision/timing:%g\".",
             get_cname(), latency, sg_precision_timing, latency);
    last_warned_latency = latency;
  }
}

void StandardLinkImpl::turn_on()
{
  if (not is_on()) {
    Resource::turn_on();
    s4u::Link::on_onoff(piface_);
    piface_.on_this_onoff(piface_);
  }
}

void StandardLinkImpl::turn_off()
{
  if (is_on()) {
    Resource::turn_off();
    s4u::Link::on_onoff(piface_);
    piface_.on_this_onoff(piface_);
    cancel_actions();
  }
}

void StandardLinkImpl::seal()
{
  if (is_sealed())
    return;

  xbt_assert(this->get_model(), "Cannot seal Link(%s) without setting the Network model first", this->get_cname());
  Resource::seal();
}

void StandardLinkImpl::on_bandwidth_change() const
{
  s4u::Link::on_bandwidth_change(piface_);
  piface_.on_this_bandwidth_change(piface_);
}

void StandardLinkImpl::set_bandwidth_profile(profile::Profile* profile)
{
  if (profile) {
    xbt_assert(bandwidth_.event == nullptr, "Cannot set a second bandwidth profile to Link %s", get_cname());
    bandwidth_.event = profile->schedule(&profile::future_evt_set, this);
  }
}

void StandardLinkImpl::set_latency_profile(profile::Profile* profile)
{
  if (profile) {
    xbt_assert(latency_.event == nullptr, "Cannot set a second latency profile to Link %s", get_cname());
    latency_.event = profile->schedule(&profile::future_evt_set, this);
  }
}

} // namespace simgrid::kernel::resource
