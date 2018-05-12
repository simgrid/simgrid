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
bool Link::isUsed()
{
  return this->pimpl_->is_used();
}

double Link::latency()
{
  return this->pimpl_->latency();
}

double Link::bandwidth()
{
  return this->pimpl_->bandwidth();
}

Link::SharingPolicy Link::sharingPolicy()
{
  return this->pimpl_->sharingPolicy();
}

double Link::getUsage()
{
  return this->pimpl_->get_constraint()->get_usage();
}

void Link::turnOn()
{
  simgrid::simix::simcall([this]() { this->pimpl_->turn_on(); });
}
void Link::turnOff()
{
  simgrid::simix::simcall([this]() { this->pimpl_->turn_off(); });
}

void* Link::getData()
{
  return this->pimpl_->getData();
}
void Link::setData(void* d)
{
  simgrid::simix::simcall([this, d]() { this->pimpl_->setData(d); });
}

void Link::setStateTrace(tmgr_trace_t trace)
{
  simgrid::simix::simcall([this, trace]() { this->pimpl_->setStateTrace(trace); });
}
void Link::setBandwidthTrace(tmgr_trace_t trace)
{
  simgrid::simix::simcall([this, trace]() { this->pimpl_->setBandwidthTrace(trace); });
}
void Link::setLatencyTrace(tmgr_trace_t trace)
{
  simgrid::simix::simcall([this, trace]() { this->pimpl_->setLatencyTrace(trace); });
}

const char* Link::getProperty(const char* key)
{
  return this->pimpl_->getProperty(key);
}
void Link::setProperty(std::string key, std::string value)
{
  simgrid::simix::simcall([this, key, value] { this->pimpl_->setProperty(key, value); });
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
  return (int)link->sharingPolicy();
}
double sg_link_bandwidth(sg_link_t link)
{
  return link->bandwidth();
}
double sg_link_latency(sg_link_t link)
{
  return link->latency();
}
void* sg_link_data(sg_link_t link)
{
  return link->getData();
}
void sg_link_data_set(sg_link_t link, void* data)
{
  link->setData(data);
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
