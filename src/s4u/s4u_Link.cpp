/* Copyright (c) 2013-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <algorithm>

#include "simgrid/s4u/Engine.hpp"
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
simgrid::xbt::signal<void(kernel::resource::NetworkAction*, kernel::resource::Action::State)>
    Link::on_communication_state_change;

Link* Link::by_name(std::string name)
{
  return Engine::get_instance()->link_by_name(name);
}

Link* Link::by_name_or_null(std::string name)
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
bool Link::is_used()
{
  return this->pimpl_->is_used();
}

double Link::get_latency()
{
  return this->pimpl_->get_latency();
}

double Link::get_bandwidth()
{
  return this->pimpl_->get_bandwidth();
}

Link::SharingPolicy Link::get_sharing_policy()
{
  return this->pimpl_->get_sharing_policy();
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
  return this->pimpl_->get_data();
}
void Link::set_data(void* d)
{
  simgrid::simix::simcall([this, d]() { this->pimpl_->set_data(d); });
}

void Link::set_state_trace(tmgr_trace_t trace)
{
  simgrid::simix::simcall([this, trace]() { this->pimpl_->set_state_trace(trace); });
}
void Link::set_bandwidth_trace(tmgr_trace_t trace)
{
  simgrid::simix::simcall([this, trace]() { this->pimpl_->set_bandwidth_trace(trace); });
}
void Link::set_latency_trace(tmgr_trace_t trace)
{
  simgrid::simix::simcall([this, trace]() { this->pimpl_->set_latency_trace(trace); });
}

const char* Link::get_property(std::string key)
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
  return simgrid::s4u::Engine::get_instance()->get_link_count();
}

sg_link_t* sg_link_list()
{
  std::vector<simgrid::s4u::Link*> links = simgrid::s4u::Engine::get_instance()->get_all_links();

  sg_link_t* res = (sg_link_t*)malloc(sizeof(sg_link_t) * links.size());
  memcpy(res, links.data(), sizeof(sg_link_t) * links.size());

  return res;
}
