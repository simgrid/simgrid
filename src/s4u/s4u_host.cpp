/* Copyright (c) 2006-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <string>
#include <functional>
#include <stdexcept>

#include <map>

#include "simgrid/s4u/Engine.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Storage.hpp"
#include "simgrid/simix.hpp"
#include "src/kernel/routing/NetPoint.hpp"
#include "src/msg/msg_private.hpp"
#include "src/simix/ActorImpl.hpp"
#include "src/simix/smx_private.hpp"
#include "src/surf/HostImpl.hpp"
#include "src/surf/cpu_interface.hpp"
#include "xbt/log.h"

XBT_LOG_EXTERNAL_CATEGORY(surf_route);

int USER_HOST_LEVEL = -1;

namespace simgrid {

namespace xbt {
template class Extendable<simgrid::s4u::Host>;
}

namespace s4u {

std::map<std::string, simgrid::s4u::Host*> host_list; // FIXME: move it to Engine

simgrid::xbt::signal<void(Host&)> Host::onCreation;
simgrid::xbt::signal<void(Host&)> Host::onDestruction;
simgrid::xbt::signal<void(Host&)> Host::onStateChange;
simgrid::xbt::signal<void(Host&)> Host::onSpeedChange;

Host::Host(const char* name)
  : name_(name)
{
  xbt_assert(Host::by_name_or_null(name) == nullptr, "Refusing to create a second host named '%s'.", name);
  host_list[name_] = this;
  new simgrid::surf::HostImpl(this);
}

Host::~Host()
{
  xbt_assert(currentlyDestroying_, "Please call h->destroy() instead of manually deleting it.");

  delete pimpl_;
  if (pimpl_netpoint != nullptr) // not removed yet by a children class
    simgrid::s4u::Engine::getInstance()->netpointUnregister(pimpl_netpoint);
  delete pimpl_cpu;
  delete mounts;
}

/** @brief Fire the required callbacks and destroy the object
 *
 * Don't delete directly an Host, call h->destroy() instead.
 *
 * This is cumbersome but this is the simplest solution to ensure that the
 * onDestruction() callback receives a valid object (because of the destructor
 * order in a class hierarchy).
 */
void Host::destroy()
{
  if (not currentlyDestroying_) {
    currentlyDestroying_ = true;
    onDestruction(*this);
    host_list.erase(name_);
    delete this;
  }
}

Host* Host::by_name(std::string name)
{
  return host_list.at(name); // Will raise a std::out_of_range if the host does not exist
}
Host* Host::by_name(const char* name)
{
  return host_list.at(std::string(name)); // Will raise a std::out_of_range if the host does not exist
}
Host* Host::by_name_or_null(const char* name)
{
  return by_name_or_null(std::string(name));
}
Host* Host::by_name_or_null(std::string name)
{
  auto host = host_list.find(name);
  return host == host_list.end() ? nullptr : host->second;
}

Host *Host::current(){
  smx_actor_t smx_proc = SIMIX_process_self();
  if (smx_proc == nullptr)
    xbt_die("Cannot call Host::current() from the maestro context");
  return smx_proc->host;
}

void Host::turnOn() {
  if (isOff()) {
    simgrid::simix::kernelImmediate([this] {
      this->extension<simgrid::simix::Host>()->turnOn();
      this->pimpl_cpu->turnOn();
      onStateChange(*this);
    });
  }
}

void Host::turnOff() {
  if (isOn()) {
    smx_actor_t self = SIMIX_process_self();
    simgrid::simix::kernelImmediate([this, self] {
      SIMIX_host_off(this, self);
      onStateChange(*this);
    });
  }
}

bool Host::isOn() {
  return this->pimpl_cpu->isOn();
}

int Host::getPstatesCount() const
{
  return this->pimpl_cpu->getNbPStates();
}

/**
 * \brief Return the list of actors attached to an host.
 *
 * \param whereto a vector in which we should push actors living on that host
 */
void Host::actorList(std::vector<ActorPtr>* whereto)
{
  for (auto& actor : this->extension<simgrid::simix::Host>()->process_list) {
    whereto->push_back(actor.ciface());
  }
}

/**
 * \brief Find a route toward another host
 *
 * \param dest [IN] where to
 * \param links [OUT] where to store the list of links (must exist, cannot be nullptr).
 * \param latency [OUT] where to store the latency experienced on the path (or nullptr if not interested)
 *                It is the caller responsibility to initialize latency to 0 (we add to provided route)
 * \pre links!=nullptr
 *
 * walk through the routing components tree and find a route between hosts
 * by calling each "get_route" function in each routing component.
 */
void Host::routeTo(Host* dest, std::vector<Link*>& links, double* latency)
{
  std::vector<surf::LinkImpl*> linkImpls;
  this->routeTo(dest, linkImpls, latency);
  for (surf::LinkImpl* const& l : linkImpls)
    links.push_back(&l->piface_);
}

/** @brief Just like Host::routeTo, but filling an array of link implementations */
void Host::routeTo(Host* dest, std::vector<surf::LinkImpl*>& links, double* latency)
{
  simgrid::kernel::routing::NetZoneImpl::getGlobalRoute(pimpl_netpoint, dest->pimpl_netpoint, links, latency);
  if (XBT_LOG_ISENABLED(surf_route, xbt_log_priority_debug)) {
    XBT_CDEBUG(surf_route, "Route from '%s' to '%s' (latency: %f):", getCname(), dest->getCname(),
               (latency == nullptr ? -1 : *latency));
    for (auto const& link : links)
      XBT_CDEBUG(surf_route, "Link %s", link->getCname());
  }
}

/** Get the properties assigned to a host */
std::map<std::string, std::string>* Host::getProperties()
{
  return simgrid::simix::kernelImmediate([this] { return this->pimpl_->getProperties(); });
}

/** Retrieve the property value (or nullptr if not set) */
const char* Host::getProperty(const char* key)
{
  return this->pimpl_->getProperty(key);
}

void Host::setProperty(std::string key, std::string value)
{
  simgrid::simix::kernelImmediate([this, key, value] { this->pimpl_->setProperty(key, value); });
}

/** Get the processes attached to the host */
void Host::getProcesses(std::vector<ActorPtr>* list)
{
  for (auto& actor : this->extension<simgrid::simix::Host>()->process_list) {
    list->push_back(actor.iface());
  }
}

/** @brief Get the peak processor speed (in flops/s), at the specified pstate  */
double Host::getPstateSpeed(int pstate_index)
{
  return simgrid::simix::kernelImmediate([this, pstate_index] {
    return this->pimpl_cpu->getPstateSpeed(pstate_index);
  });
}

/** @brief Get the peak processor speed (in flops/s), at the current pstate */
double Host::getSpeed()
{
  return pimpl_cpu->getSpeed(1.0);
}

/** @brief Returns the number of core of the processor. */
int Host::getCoreCount()
{
  return pimpl_cpu->coreCount();
}

/** @brief Set the pstate at which the host should run */
void Host::setPstate(int pstate_index)
{
  simgrid::simix::kernelImmediate([this, pstate_index] {
      this->pimpl_cpu->setPState(pstate_index);
  });
}
/** @brief Retrieve the pstate at which the host is currently running */
int Host::getPstate()
{
  return this->pimpl_cpu->getPState();
}

/**
 * \ingroup simix_storage_management
 * \brief Returns the list of storages attached to an host.
 * \return a vector containing all storages attached to the host
 */
void Host::getAttachedStorages(std::vector<const char*>* storages)
{
  simgrid::simix::kernelImmediate([this, storages] {
     this->pimpl_->getAttachedStorageList(storages);
  });
}

std::unordered_map<std::string, Storage*> const& Host::getMountedStorages()
{
  if (mounts == nullptr) {
    mounts = new std::unordered_map<std::string, Storage*>();
    for (auto const& m : this->pimpl_->storage_) {
      mounts->insert({m.first, &m.second->piface_});
    }
  }
  return *mounts;
}

void Host::execute(double flops)
{
  Host* host_list[1]   = {this};
  double flops_list[1] = {flops};
  smx_activity_t s     = simcall_execution_parallel_start(nullptr /*name*/, 1, host_list, flops_list,
                                                      nullptr /*comm_sizes */, -1.0, -1 /*timeout*/);
  simcall_execution_wait(s);
}

double Host::getLoad()
{
  return this->pimpl_cpu->getLoad();
}

} // namespace simgrid
} // namespace s4u
