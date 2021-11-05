/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/surf/SplitDuplexLinkImpl.hpp"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(res_network);

/*********
 * Model *
 *********/

namespace simgrid {
namespace kernel {
namespace resource {

SplitDuplexLinkImpl::SplitDuplexLinkImpl(const std::string& name, LinkImpl* link_up, LinkImpl* link_down)
    : LinkImplIntf(name), piface_(this), link_up_(link_up), link_down_(link_down)
{
}

bool SplitDuplexLinkImpl::is_used() const
{
  return link_up_->is_used() || link_down_->is_used();
}

void SplitDuplexLinkImpl::set_sharing_policy(s4u::Link::SharingPolicy policy, const s4u::NonLinearResourceCb& cb)
{
  xbt_assert(policy != s4u::Link::SharingPolicy::SPLITDUPLEX && policy != s4u::Link::SharingPolicy::WIFI,
             "Invalid sharing policy for split-duplex links");
  link_up_->set_sharing_policy(policy, cb);
  link_down_->set_sharing_policy(policy, cb);
}

void SplitDuplexLinkImpl::set_bandwidth(double value)
{
  link_up_->set_bandwidth(value);
  link_down_->set_bandwidth(value);
}

void SplitDuplexLinkImpl::set_latency(double value)
{
  link_up_->set_latency(value);
  link_down_->set_latency(value);
}

void SplitDuplexLinkImpl::turn_on()
{
  link_up_->turn_on();
  link_down_->turn_on();
}

void SplitDuplexLinkImpl::turn_off()
{
  link_up_->turn_off();
  link_down_->turn_off();
}

void SplitDuplexLinkImpl::apply_event(profile::Event* event, double value)
{
  link_up_->apply_event(event, value);
  link_down_->apply_event(event, value);
}

void SplitDuplexLinkImpl::seal()
{
  if (is_sealed())
    return;

  link_up_->seal();
  link_down_->seal();

  Resource::seal();
}

void SplitDuplexLinkImpl::set_bandwidth_profile(profile::Profile* profile)
{
  link_up_->set_bandwidth_profile(profile);
  link_down_->set_bandwidth_profile(profile);
}

void SplitDuplexLinkImpl::set_latency_profile(profile::Profile* profile)
{
  link_up_->set_latency_profile(profile);
  link_down_->set_latency_profile(profile);
}

void SplitDuplexLinkImpl::set_concurrency_limit(int limit) const
{
  link_up_->set_concurrency_limit(limit);
  link_down_->set_concurrency_limit(limit);
}

} // namespace resource
} // namespace kernel
} // namespace simgrid
