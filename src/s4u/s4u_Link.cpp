/* Copyright (c) 2013-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <algorithm>

#include "simgrid/s4u/Link.hpp"
#include "simgrid/sg_config.hpp"
#include "simgrid/simix.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "src/surf/network_interface.hpp"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_link, s4u, "Logging specific to the S4U links");

namespace simgrid {
namespace s4u {

simgrid::xbt::signal<void(Link&)> Link::on_creation;
simgrid::xbt::signal<void(Link&)> Link::on_destruction;
simgrid::xbt::signal<void(Link&)> Link::on_state_change;
simgrid::xbt::signal<void(Link&)> Link::on_bandwidth_change;
simgrid::xbt::signal<void(kernel::resource::NetworkAction*, Host* src, Host* dst)> Link::on_communicate;
simgrid::xbt::signal<void(kernel::resource::NetworkAction*)> Link::on_communication_state_change;

Link* Link::by_name(const char* name)
{
  kernel::resource::LinkImpl* res = kernel::resource::LinkImpl::byName(name);
  if (res == nullptr)
    return nullptr;
  return &res->piface_;
}
const std::string& Link::get_name() const
{
  return this->pimpl_->get_name();
}
const char* Link::get_cname() const
{
  return this->pimpl_->get_cname();
}
const char* Link::name()
{
  return get_cname();
}
bool Link::is_used()
{
  return this->pimpl_->is_used();
}

double Link::get_latency()
{
  return this->pimpl_->latency();
}

double Link::get_bandwidth()
{
  return this->pimpl_->bandwidth();
}

Link::SharingPolicy Link::get_sharing_policy()
{
  return this->pimpl_->sharingPolicy();
}

double Link::get_usage()
{
  return this->pimpl_->get_constraint()->get_usage();
}

void Link::turn_on()
{
  simgrid::simix::simcall([this]() { this->pimpl_->turn_on(); });
}
void Link::turn_off()
{
  simgrid::simix::simcall([this]() { this->pimpl_->turn_off(); });
}

void* Link::get_data()
{
  return this->pimpl_->getData();
}
void Link::set_data(void* d)
{
  simgrid::simix::simcall([this, d]() { this->pimpl_->setData(d); });
}

void Link::set_state_trace(tmgr_trace_t trace)
{
  simgrid::simix::simcall([this, trace]() { this->pimpl_->setStateTrace(trace); });
}
void Link::set_bandwidth_trace(tmgr_trace_t trace)
{
  simgrid::simix::simcall([this, trace]() { this->pimpl_->setBandwidthTrace(trace); });
}
void Link::set_latency_trace(tmgr_trace_t trace)
{
  simgrid::simix::simcall([this, trace]() { this->pimpl_->setLatencyTrace(trace); });
}

const char* Link::get_property(const char* key)
{
  return this->pimpl_->get_property(key);
}
void Link::set_property(std::string key, std::string value)
{
  simgrid::simix::simcall([this, key, value] { this->pimpl_->set_property(key, value); });
}
} // namespace s4u
} // namespace simgrid

/* **************************** Public C interface *************************** */

const char* sg_link_name(sg_link_t link)
{
  return link->get_cname();
}
sg_link_t sg_link_by_name(const char* name)
{
  return simgrid::s4u::Link::by_name(name);
}

int sg_link_is_shared(sg_link_t link)
{
  return (int)link->get_sharing_policy();
}
double sg_link_bandwidth(sg_link_t link)
{
  return link->get_bandwidth();
}
double sg_link_latency(sg_link_t link)
{
  return link->get_latency();
}
void* sg_link_data(sg_link_t link)
{
  return link->get_data();
}
void sg_link_data_set(sg_link_t link, void* data)
{
  link->set_data(data);
}
int sg_link_count()
{
  return simgrid::kernel::resource::LinkImpl::linksCount();
}
sg_link_t* sg_link_list()
{
  simgrid::kernel::resource::LinkImpl** list = simgrid::kernel::resource::LinkImpl::linksList();
  sg_link_t* res                             = (sg_link_t*)list; // Use the same memory area

  int size = sg_link_count();
  for (int i = 0; i < size; i++)
    res[i]   = &(list[i]->piface_); // Convert each entry into its interface

  return res;
}
void sg_link_exit()
{
  simgrid::kernel::resource::LinkImpl::linksExit();
}
