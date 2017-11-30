/* Copyright (c) 2013-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <algorithm>

#include "simgrid/s4u/Link.hpp"
#include "simgrid/sg_config.h"
#include "simgrid/simix.hpp"
#include "src/kernel/lmm/maxmin.hpp"
#include "src/surf/network_interface.hpp"
#include "xbt/log.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(s4u_link, s4u, "Logging specific to the S4U links");

/*********
 * C API *
 *********/

extern "C" {

const char* sg_link_name(sg_link_t link)
{
  return link->getCname();
}
sg_link_t sg_link_by_name(const char* name)
{
  return simgrid::s4u::Link::byName(name);
}

int sg_link_is_shared(sg_link_t link)
{
  return link->sharingPolicy();
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
  return simgrid::surf::LinkImpl::linksCount();
}
sg_link_t* sg_link_list()
{
  simgrid::surf::LinkImpl** list = simgrid::surf::LinkImpl::linksList();
  sg_link_t* res                 = (sg_link_t*)list; // Use the same memory area

  int size = sg_link_count();
  for (int i = 0; i < size; i++)
    res[i] = &(list[i]->piface_); // Convert each entry into its interface

  return res;
}
void sg_link_exit()
{
  simgrid::surf::LinkImpl::linksExit();
}
}

/***********
 * C++ API *
 ***********/

namespace simgrid {
namespace s4u {
Link* Link::byName(const char* name)
{
  surf::LinkImpl* res = surf::LinkImpl::byName(name);
  if (res == nullptr)
    return nullptr;
  return &res->piface_;
}
const std::string& Link::getName() const
{
  return this->pimpl_->getName();
}
const char* Link::getCname() const
{
  return this->pimpl_->getCname();
}
const char* Link::name()
{
  return getCname();
}
bool Link::isUsed()
{
  return this->pimpl_->isUsed();
}

double Link::latency()
{
  return this->pimpl_->latency();
}

double Link::bandwidth()
{
  return this->pimpl_->bandwidth();
}

int Link::sharingPolicy()
{
  return this->pimpl_->sharingPolicy();
}

double Link::getUsage()
{
  return this->pimpl_->constraint()->get_usage();
}

void Link::turnOn()
{
  simgrid::simix::kernelImmediate([this]() {
    this->pimpl_->turnOn();
  });
}
void Link::turnOff()
{
  simgrid::simix::kernelImmediate([this]() {
    this->pimpl_->turnOff();
  });
}

void* Link::getData()
{
  return this->pimpl_->getData();
}
void Link::setData(void* d)
{
  simgrid::simix::kernelImmediate([this, d]() {
    this->pimpl_->setData(d);
  });
}

void Link::setStateTrace(tmgr_trace_t trace)
{
  simgrid::simix::kernelImmediate([this, trace]() {
    this->pimpl_->setStateTrace(trace);
  });
}
void Link::setBandwidthTrace(tmgr_trace_t trace)
{
  simgrid::simix::kernelImmediate([this, trace]() {
    this->pimpl_->setBandwidthTrace(trace);
  });
}
void Link::setLatencyTrace(tmgr_trace_t trace)
{
  simgrid::simix::kernelImmediate([this, trace]() {
    this->pimpl_->setLatencyTrace(trace);
  });
}

const char* Link::getProperty(const char* key)
{
  return this->pimpl_->getProperty(key);
}
void Link::setProperty(std::string key, std::string value)
{
  simgrid::simix::kernelImmediate([this, key, value] { this->pimpl_->setProperty(key, value); });
}

/*************
 * Callbacks *
 *************/
simgrid::xbt::signal<void(s4u::Link&)> Link::onCreation;
simgrid::xbt::signal<void(s4u::Link&)> Link::onDestruction;
simgrid::xbt::signal<void(s4u::Link&)> Link::onStateChange;
simgrid::xbt::signal<void(surf::NetworkAction*, s4u::Host* src, s4u::Host* dst)> Link::onCommunicate;
simgrid::xbt::signal<void(surf::NetworkAction*)> Link::onCommunicationStateChange;
}
}
