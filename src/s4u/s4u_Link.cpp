/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/Exception.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <simgrid/s4u/Link.hpp>
#include <simgrid/simix.hpp>
#include <xbt/config.hpp>
#include <xbt/parse_units.hpp>

#include "src/kernel/resource/LinkImpl.hpp"
#include "src/kernel/resource/SplitDuplexLinkImpl.hpp"
#include "src/kernel/resource/WifiLinkImpl.hpp"

namespace simgrid {

template class xbt::Extendable<s4u::Link>;

namespace s4u {

xbt::signal<void(Link&)> Link::on_creation;
xbt::signal<void(Link const&)> Link::on_destruction;
xbt::signal<void(Link const&)> Link::on_state_change;
xbt::signal<void(Link const&)> Link::on_bandwidth_change;
xbt::signal<void(kernel::resource::NetworkAction&, kernel::resource::Action::State)>
    Link::on_communication_state_change;

Link* Link::by_name(const std::string& name)
{
  return Engine::get_instance()->link_by_name(name);
}

kernel::resource::StandardLinkImpl* Link::get_impl() const
{
  auto* link_impl = dynamic_cast<kernel::resource::StandardLinkImpl*>(pimpl_);
  xbt_assert(link_impl != nullptr, "Impossible to get a LinkImpl* from link. %s.",
             (get_sharing_policy() == SharingPolicy::SPLITDUPLEX
                  ? "For a Split-Duplex link, you should call this method to each UP/DOWN member"
                  : "Please report this bug"));
  return link_impl;
}

Link* Link::by_name_or_null(const std::string& name)
{
  return Engine::get_instance()->link_by_name_or_null(name);
}

const std::string& Link::get_name() const
{
  return this->pimpl_->get_name();
}
const char* Link::get_cname() const
{
  return this->pimpl_->get_cname();
}
bool Link::is_used() const
{
  return this->pimpl_->is_used();
}

bool Link::is_shared() const
{
  return this->pimpl_->get_sharing_policy() != SharingPolicy::FATPIPE;
}

double Link::get_latency() const
{
  return this->pimpl_->get_latency();
}

Link* Link::set_latency(double value)
{
  kernel::actor::simcall([this, value] { pimpl_->set_latency(value); });
  return this;
}

Link* Link::set_latency(const std::string& value)
{
  double d_value = 0.0;
  try {
    d_value = xbt_parse_get_time("", 0, value, "");
  } catch (const simgrid::ParseError&) {
    throw std::invalid_argument(std::string("Impossible to set latency for link: ") + get_name() +
                                std::string(". Invalid value: ") + value);
  }
  return set_latency(d_value);
}

double Link::get_bandwidth() const
{
  return this->pimpl_->get_bandwidth();
}

Link* Link::set_bandwidth(double value)
{
  kernel::actor::simcall([this, value] { pimpl_->set_bandwidth(value); });
  return this;
}

Link* Link::set_sharing_policy(Link::SharingPolicy policy, const NonLinearResourceCb& cb)
{
  if (policy == SharingPolicy::SPLITDUPLEX || policy == SharingPolicy::WIFI)
    throw std::invalid_argument(std::string("Impossible to set wifi or split-duplex for the link: ") + get_name() +
                                std::string(". Use appropriate create function in NetZone."));

  kernel::actor::simcall([this, policy, &cb] { pimpl_->set_sharing_policy(policy, cb); });
  return this;
}
Link::SharingPolicy Link::get_sharing_policy() const
{
  return this->pimpl_->get_sharing_policy();
}

void Link::set_host_wifi_rate(const s4u::Host* host, int level) const
{
  auto* wlink = dynamic_cast<kernel::resource::WifiLinkImpl*>(pimpl_);
  xbt_assert(wlink != nullptr, "Link %s does not seem to be a wifi link.", get_cname());
  wlink->set_host_rate(host, level);
}

Link* Link::set_concurrency_limit(int limit)
{
  kernel::actor::simcall([this, limit] { pimpl_->set_concurrency_limit(limit); });
  return this;
}

double Link::get_usage() const
{
  return this->pimpl_->get_constraint()->get_usage();
}

void Link::turn_on()
{
  kernel::actor::simcall([this]() { this->pimpl_->turn_on(); });
}
void Link::turn_off()
{
  kernel::actor::simcall([this]() { this->pimpl_->turn_off(); });
}
Link* Link::seal()
{
  kernel::actor::simcall([this]() { this->pimpl_->seal(); });
  s4u::Link::on_creation(*this); // notify the signal
  return this;
}

bool Link::is_on() const
{
  return this->pimpl_->is_on();
}

Link* Link::set_state_profile(kernel::profile::Profile* profile)
{
  xbt_assert(not pimpl_->is_sealed(), "Cannot set a state profile once the Link is sealed");
  kernel::actor::simcall([this, profile]() { this->pimpl_->set_state_profile(profile); });
  return this;
}

Link* Link::set_bandwidth_profile(kernel::profile::Profile* profile)
{
  xbt_assert(not pimpl_->is_sealed(), "Cannot set a bandwidth profile once the Link is sealed");
  kernel::actor::simcall([this, profile]() { this->pimpl_->set_bandwidth_profile(profile); });
  return this;
}

Link* Link::set_latency_profile(kernel::profile::Profile* profile)
{
  xbt_assert(not pimpl_->is_sealed(), "Cannot set a latency profile once the Link is sealed");
  kernel::actor::simcall([this, profile]() { this->pimpl_->set_latency_profile(profile); });
  return this;
}

const char* Link::get_property(const std::string& key) const
{
  return this->pimpl_->get_property(key);
}
Link* Link::set_property(const std::string& key, const std::string& value)
{
  kernel::actor::simcall([this, &key, &value] { this->pimpl_->set_property(key, value); });
  return this;
}

const std::unordered_map<std::string, std::string>* Link::get_properties() const
{
  return this->pimpl_->get_properties();
}

Link* Link::set_properties(const std::unordered_map<std::string, std::string>& properties)
{
  kernel::actor::simcall([this, &properties] { this->pimpl_->set_properties(properties); });
  return this;
}

Link* SplitDuplexLink::get_link_up() const
{
  const auto* pimpl = dynamic_cast<kernel::resource::SplitDuplexLinkImpl*>(pimpl_);
  xbt_assert(pimpl, "Requesting link_up from a non split-duplex link: %s", get_cname());
  return pimpl->get_link_up();
}

Link* SplitDuplexLink::get_link_down() const
{
  const auto* pimpl = dynamic_cast<kernel::resource::SplitDuplexLinkImpl*>(pimpl_);
  xbt_assert(pimpl, "Requesting link_down from a non split-duplex link: %s", get_cname());
  return pimpl->get_link_down();
}

SplitDuplexLink* SplitDuplexLink::by_name(const std::string& name)
{
  return Engine::get_instance()->split_duplex_link_by_name(name);
}

} // namespace s4u
} // namespace simgrid

/* **************************** Public C interface *************************** */

const char* sg_link_get_name(const_sg_link_t link)
{
  return link->get_cname();
}

sg_link_t sg_link_by_name(const char* name)
{
  return simgrid::s4u::Link::by_name(name);
}

int sg_link_is_shared(const_sg_link_t link)
{
  return link->is_shared();
}

double sg_link_get_bandwidth(const_sg_link_t link)
{
  return link->get_bandwidth();
}

void sg_link_set_bandwidth(sg_link_t link, double value)
{
  link->set_bandwidth(value);
}

double sg_link_get_latency(const_sg_link_t link)
{
  return link->get_latency();
}

void sg_link_set_latency(sg_link_t link, double value)
{
  link->set_latency(value);
}

void* sg_link_get_data(const_sg_link_t link)
{
  return link->get_data();
}

void sg_link_set_data(sg_link_t link, void* data)
{
  link->set_data(data);
}

size_t sg_link_count()
{
  return simgrid::s4u::Engine::get_instance()->get_link_count();
}

sg_link_t* sg_link_list()
{
  std::vector<simgrid::s4u::Link*> links = simgrid::s4u::Engine::get_instance()->get_all_links();

  auto* res = xbt_new(sg_link_t, links.size());
  std::copy(begin(links), end(links), res);

  return res;
}
