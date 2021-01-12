/* Copyright (c) 2013-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <algorithm>

#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Link.hpp"
#include "simgrid/sg_config.hpp"
#include "simgrid/simix.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/network_wifi.hpp"
#include "xbt/log.h"

namespace simgrid {

template class xbt::Extendable<s4u::Link>;

namespace s4u {

xbt::signal<void(Link&)> Link::on_creation;
xbt::signal<void(Link const&)> Link::on_destruction;
xbt::signal<void(Link const&)> Link::on_state_change;
xbt::signal<void(Link const&)> Link::on_bandwidth_change;
xbt::signal<void(kernel::resource::NetworkAction&)> Link::on_communicate;
xbt::signal<void(kernel::resource::NetworkAction&, kernel::resource::Action::State)>
    Link::on_communication_state_change;

Link* Link::by_name(const std::string& name)
{
  return Engine::get_instance()->link_by_name(name);
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

double Link::get_latency() const
{
  return this->pimpl_->get_latency();
}

void Link::set_latency(double value)
{
  kernel::actor::simcall([this, value] { pimpl_->set_latency(value); });
}

double Link::get_bandwidth() const
{
  return this->pimpl_->get_bandwidth();
}

void Link::set_bandwidth(double value)
{
  kernel::actor::simcall([this, value] { pimpl_->set_bandwidth(value); });
}

Link::SharingPolicy Link::get_sharing_policy() const
{
  return this->pimpl_->get_sharing_policy();
}

void Link::set_host_wifi_rate(const s4u::Host* host, int level) const
{
  xbt_assert(pimpl_->get_sharing_policy() == Link::SharingPolicy::WIFI, "Link %s does not seem to be a wifi link.",
             get_cname());
  auto* wlink = dynamic_cast<kernel::resource::NetworkWifiLink*>(pimpl_);
  xbt_assert(wlink != nullptr, "Cannot convert link %s into a wifi link.", get_cname());
  wlink->set_host_rate(host, level);
}

double Link::get_usage() const
{
  return this->pimpl_->get_constraint()->get_usage();
}

void Link::turn_on()
{
  simgrid::kernel::actor::simcall([this]() { this->pimpl_->turn_on(); });
}
void Link::turn_off()
{
  simgrid::kernel::actor::simcall([this]() { this->pimpl_->turn_off(); });
}

bool Link::is_on() const
{
  return this->pimpl_->is_on();
}

void Link::set_state_profile(kernel::profile::Profile* profile)
{
  simgrid::kernel::actor::simcall([this, profile]() { this->pimpl_->set_state_profile(profile); });
}
void Link::set_bandwidth_profile(kernel::profile::Profile* profile)
{
  simgrid::kernel::actor::simcall([this, profile]() { this->pimpl_->set_bandwidth_profile(profile); });
}
void Link::set_latency_profile(kernel::profile::Profile* trace)
{
  simgrid::kernel::actor::simcall([this, trace]() { this->pimpl_->set_latency_profile(trace); });
}

const char* Link::get_property(const std::string& key) const
{
  return this->pimpl_->get_property(key);
}
void Link::set_property(const std::string& key, const std::string& value)
{
  simgrid::kernel::actor::simcall([this, &key, &value] { this->pimpl_->set_property(key, value); });
}
} // namespace s4u
} // namespace simgrid

/* **************************** Public C interface *************************** */

const char* sg_link_get_name(const_sg_link_t link)
{
  return link->get_cname();
}

const char* sg_link_name(const_sg_link_t link) // XBT_ATTRIB_DEPRECATED_v330
{
  return sg_link_get_name(link);
}

sg_link_t sg_link_by_name(const char* name)
{
  return simgrid::s4u::Link::by_name(name);
}

int sg_link_is_shared(const_sg_link_t link)
{
  return link->get_sharing_policy() != simgrid::s4u::Link::SharingPolicy::FATPIPE;
}

double sg_link_get_bandwidth(const_sg_link_t link)
{
  return link->get_bandwidth();
}

void sg_link_set_bandwidth(sg_link_t link, double value)
{
  return link->set_bandwidth(value);
}

double sg_link_bandwidth(const_sg_link_t link) // XBT_ATTRIB_DEPRECATED_v330
{
  return sg_link_get_bandwidth(link);
}

void sg_link_bandwidth_set(sg_link_t link, double value) // XBT_ATTRIB_DEPRECATED_v330
{
  return sg_link_set_bandwidth(link, value);
}

double sg_link_get_latency(const_sg_link_t link)
{
  return link->get_latency();
}

void sg_link_set_latency(sg_link_t link, double value)
{
  return link->set_latency(value);
}

double sg_link_latency(const_sg_link_t link) // XBT_ATTRIB_DEPRECATED_v330
{
  return sg_link_get_latency(link);
}

void sg_link_latency_set(sg_link_t link, double value) // XBT_ATTRIB_DEPRECATED_v330
{
  return sg_link_set_latency(link, value);
}

void* sg_link_get_data(const_sg_link_t link)
{
  return link->get_data();
}

void sg_link_set_data(sg_link_t link, void* data)
{
  link->set_data(data);
}

void* sg_link_data(const_sg_link_t link) // XBT_ATTRIB_DEPRECATED_v330
{
  return sg_link_get_data(link);
}

void sg_link_data_set(sg_link_t link, void* data) // XBT_ATTRIB_DEPRECATED_v330
{
  sg_link_set_data(link, data);
}

int sg_link_count()
{
  return simgrid::s4u::Engine::get_instance()->get_link_count();
}

sg_link_t* sg_link_list()
{
  std::vector<simgrid::s4u::Link*> links = simgrid::s4u::Engine::get_instance()->get_all_links();

  sg_link_t* res = xbt_new(sg_link_t, links.size());
  memcpy(res, links.data(), sizeof(sg_link_t) * links.size());

  return res;
}
