/* Copyright (c) 2019-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/s4u/Comm.hpp>
#include <simgrid/s4u/Host.hpp>

#include "src/kernel/activity/CommImpl.hpp"
#include "src/kernel/resource/WifiLinkImpl.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(res_network);

namespace simgrid::kernel::resource {

/************
 * Resource *
 ************/
static void update_bw_comm_start(const s4u::Comm& comm)
{
  const auto* pimpl = static_cast<activity::CommImpl*>(comm.get_impl());

  auto const* actionWifi = dynamic_cast<const kernel::resource::WifiLinkAction*>(pimpl->model_action_);
  if (actionWifi == nullptr)
    return;

  if (auto* link_src = actionWifi->get_src_link()) {
    link_src->inc_active_flux();
  }
  if (auto* link_dst = actionWifi->get_dst_link()) {
    link_dst->inc_active_flux();
  }
}

WifiLinkImpl::WifiLinkImpl(const std::string& name, const std::vector<double>& bandwidths, lmm::System* system)
    : StandardLinkImpl(name)
{
  this->set_constraint(system->constraint_new(this, 1));
  for (auto bandwidth : bandwidths)
    bandwidths_.push_back({bandwidth, 1.0, nullptr});
  s4u::Comm::on_start_cb(&update_bw_comm_start);
  s4u::Link::on_communication_state_change_cb(&update_bw_comm_end);
}

void WifiLinkImpl::set_host_rate(const s4u::Host* host, int rate_level)
{
  host_rates_[host->get_name()] = rate_level;
}

double WifiLinkImpl::get_host_rate(const s4u::Host* host) const
{
  auto host_rates_it = host_rates_.find(host->get_name());

  if (host_rates_it == host_rates_.end())
    return -1;

  int rate_id = host_rates_it->second;
  xbt_assert(rate_id >= 0,
             "Negative host wifi rate levels are invalid but host '%s' uses %d as a rate level on link '%s'",
             host->get_cname(), rate_id, this->get_cname());
  xbt_assert(rate_id < (int)bandwidths_.size(),
             "Link '%s' only has %zu wifi rate levels, so the provided level %d is invalid for host '%s'.",
             this->get_cname(), bandwidths_.size(), rate_id, host->get_cname());

  Metric rate = bandwidths_[rate_id];
  return rate.peak * rate.scale;
}

s4u::Link::SharingPolicy WifiLinkImpl::get_sharing_policy() const
{
  return s4u::Link::SharingPolicy::WIFI;
}

size_t WifiLinkImpl::get_host_count() const
{
  return host_rates_.size();
}

double WifiLinkImpl::wifi_link_dynamic_sharing(const WifiLinkImpl& link, double /*capacity*/, int /*n*/)
{
  double ratio = link.get_max_ratio();
  XBT_DEBUG("New ratio value concurrency %d: %lf of link capacity on link %s", link.nb_active_flux_, ratio, link.get_name().c_str());
  return ratio;
}

void WifiLinkImpl::inc_active_flux()
{
  xbt_assert(nb_active_flux_ >= 0, "Negative nb_active_flux should not exist");
  nb_active_flux_++;
}

void WifiLinkImpl::dec_active_flux()
{
  xbt_assert(nb_active_flux_ > 0, "Negative nb_active_flux should not exist");
  nb_active_flux_--;
}

void WifiLinkImpl::update_bw_comm_end(const simgrid::kernel::resource::NetworkAction& action,
                                      simgrid::kernel::resource::Action::State /*state*/)
{
  if (action.get_state() != kernel::resource::Action::State::FINISHED)
    return;

  auto const* actionWifi = dynamic_cast<const simgrid::kernel::resource::WifiLinkAction*>(&action);
  if (actionWifi == nullptr)
    return;

  if (auto* link_src = actionWifi->get_src_link()) {
    link_src->dec_active_flux();
  }
  if (auto* link_dst = actionWifi->get_dst_link()) {
    link_dst->dec_active_flux();
  }
}

double WifiLinkImpl::get_max_ratio() const
{
  double new_peak;
  if (nb_active_flux_ > conc_lim_) {
    new_peak = (nb_active_flux_-conc_lim_) * co_acc_ + x0_;
    XBT_DEBUG("Wi-Fi link peak=(%d-%d)*%lf+%lf=%lf", nb_active_flux_, conc_lim_, co_acc_, x0_, new_peak);
  } else {
    new_peak = x0_;
    XBT_DEBUG("Wi-Fi link peak=%lf", x0_);
  }
  // should be the new maximum bandwidth ratio (comparison between max throughput without concurrency and with it)
  double propCap = new_peak / x0_;

  return propCap;
}

bool WifiLinkImpl::toggle_callback()
{
  if (not use_callback_) {
      XBT_DEBUG("Activate throughput reduction mechanism");
    use_callback_ = true;
    this->set_sharing_policy(
        simgrid::s4u::Link::SharingPolicy::WIFI,
        std::bind(&wifi_link_dynamic_sharing, std::cref(*this), std::placeholders::_1, std::placeholders::_2));
  }
  return use_callback_;
}

void WifiLinkImpl::set_latency(double value)
{
  xbt_assert(value == 0, "Latency cannot be set for WiFi Links.");
}
} // namespace simgrid::kernel::resource
