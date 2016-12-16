/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <string>
#include <functional>
#include <stdexcept>

#include <unordered_map>

#include "simgrid/s4u/host.hpp"
#include "simgrid/s4u/storage.hpp"
#include "simgrid/simix.hpp"
#include "src/kernel/routing/NetCard.hpp"
#include "src/msg/msg_private.h"
#include "src/simix/ActorImpl.hpp"
#include "src/simix/smx_private.h"
#include "src/surf/HostImpl.hpp"
#include "src/surf/cpu_interface.hpp"
#include "xbt/log.h"

XBT_LOG_EXTERNAL_CATEGORY(surf_route);

std::unordered_map<std::string, simgrid::s4u::Host*> host_list; // FIXME: move it to Engine

int MSG_HOST_LEVEL = -1;
int USER_HOST_LEVEL = -1;

namespace simgrid {

namespace xbt {
template class Extendable<simgrid::s4u::Host>;
}

namespace s4u {

simgrid::xbt::signal<void(Host&)> Host::onCreation;
simgrid::xbt::signal<void(Host&)> Host::onDestruction;
simgrid::xbt::signal<void(Host&)> Host::onStateChange;

Host::Host(const char* name)
  : name_(name)
{
  xbt_assert(Host::by_name_or_null(name) == nullptr, "Refusing to create a second host named '%s'.", name);
  host_list[name_] = this;
}

Host::~Host()
{
  xbt_assert(currentlyDestroying_, "Please call h->destroy() instead of manually deleting it.");

  delete pimpl_;
  delete pimpl_cpu;
  delete pimpl_netcard;
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
  if (!currentlyDestroying_) {
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
Host* Host::by_name_or_null(const char* name)
{
  return by_name_or_null(std::string(name));
}
Host* Host::by_name_or_null(std::string name)
{
  if (host_list.find(name) == host_list.end())
    return nullptr;
  return host_list.at(name);
}

Host *Host::current(){
  smx_actor_t smx_proc = SIMIX_process_self();
  if (smx_proc == nullptr)
    xbt_die("Cannot call Host::current() from the maestro context");
  return smx_proc->host;
}

void Host::turnOn() {
  if (isOff()) {
    simgrid::simix::kernelImmediate([&]{
      this->extension<simgrid::simix::Host>()->turnOn();
      this->pimpl_cpu->turnOn();
    });
  }
}

void Host::turnOff() {
  if (isOn()) {
    simgrid::simix::kernelImmediate(std::bind(SIMIX_host_off, this, SIMIX_process_self()));
  }
}

bool Host::isOn() {
  return this->pimpl_cpu->isOn();
}

int Host::pstatesCount() const {
  return this->pimpl_cpu->getNbPStates();
}

/**
 * \brief Find a route toward another host
 *
 * \param dest [IN] where to
 * \param route [OUT] where to store the list of links (must exist, cannot be nullptr).
 * \param latency [OUT] where to store the latency experienced on the path (or nullptr if not interested)
 *                It is the caller responsibility to initialize latency to 0 (we add to provided route)
 * \pre route!=nullptr
 *
 * walk through the routing components tree and find a route between hosts
 * by calling each "get_route" function in each routing component.
 */
void Host::routeTo(Host* dest, std::vector<Link*>* links, double* latency)
{
  simgrid::kernel::routing::NetZoneImpl::getGlobalRoute(pimpl_netcard, dest->pimpl_netcard, links, latency);
  if (XBT_LOG_ISENABLED(surf_route, xbt_log_priority_debug)) {
    XBT_CDEBUG(surf_route, "Route from '%s' to '%s' (latency: %f):", cname(), dest->cname(),
               (latency == nullptr ? -1 : *latency));
    for (auto link : *links)
      XBT_CDEBUG(surf_route, "Link %s", link->getName());
  }
}

boost::unordered_map<std::string, Storage*> const& Host::mountedStorages() {
  if (mounts == nullptr) {
    mounts = new boost::unordered_map<std::string, Storage*> ();

    xbt_dict_t dict = this->mountedStoragesAsDict();

    xbt_dict_cursor_t cursor;
    char *mountname;
    char *storagename;
    xbt_dict_foreach(dict, cursor, mountname, storagename) {
      mounts->insert({mountname, &Storage::byName(storagename)});
    }
    xbt_dict_free(&dict);
  }

  return *mounts;
}

/** Get the properties assigned to a host */
xbt_dict_t Host::properties() {
  return simgrid::simix::kernelImmediate([&] { return this->pimpl_->getProperties(); });
}

/** Retrieve the property value (or nullptr if not set) */
const char*Host::property(const char*key) {
  return this->pimpl_->getProperty(key);
}
void Host::setProperty(const char*key, const char *value){
  simgrid::simix::kernelImmediate([&] { this->pimpl_->setProperty(key, value); });
}

/** Get the processes attached to the host */
xbt_swag_t Host::processes()
{
  return simgrid::simix::kernelImmediate([&]() {
    return this->extension<simgrid::simix::Host>()->process_list;
  });
}

/** Get the peak power of a host */
double Host::getPstateSpeedCurrent()
{
  return simgrid::simix::kernelImmediate([&] {
    return this->pimpl_cpu->getPstateSpeedCurrent();
  });
}

/** Get one power peak (in flops/s) of a host at a given pstate */
double Host::getPstateSpeed(int pstate_index)
{
  return simgrid::simix::kernelImmediate([&] {
    return this->pimpl_cpu->getPstateSpeed(pstate_index);
  });
}

/** @brief Get the speed of the cpu associated to a host */
double Host::speed() {
  return pimpl_cpu->getSpeed(1.0);
}
/** @brief Returns the number of core of the processor. */
int Host::coreCount() {
  return pimpl_cpu->coreCount();
}

/** @brief Set the pstate at which the host should run */
void Host::setPstate(int pstate_index)
{
  simgrid::simix::kernelImmediate(std::bind(
      &simgrid::surf::Cpu::setPState, pimpl_cpu, pstate_index
  ));
}
/** @brief Retrieve the pstate at which the host is currently running */
int Host::pstate()
{
  return pimpl_cpu->getPState();
}

/**
 * \ingroup simix_storage_management
 * \brief Returns the list of storages mounted on an host.
 * \return a dict containing all storages mounted on the host
 */
xbt_dict_t Host::mountedStoragesAsDict()
{
  return simgrid::simix::kernelImmediate([&] { return this->pimpl_->getMountedStorageList(); });
}

/**
 * \ingroup simix_storage_management
 * \brief Returns the list of storages attached to an host.
 * \return a dict containing all storages attached to the host
 */
xbt_dynar_t Host::attachedStorages()
{
  return simgrid::simix::kernelImmediate([&] { return this->pimpl_->getAttachedStorageList(); });
}

} // namespace simgrid
} // namespace s4u
