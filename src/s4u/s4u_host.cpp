/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <string>
#include <functional>
#include <stdexcept>

#include <unordered_map>

#include "simgrid/simix.hpp"
#include "src/surf/HostImpl.hpp"
#include "xbt/log.h"
#include "src/simix/ActorImpl.hpp"
#include "src/simix/smx_private.h"
#include "src/surf/cpu_interface.hpp"
#include "simgrid/s4u/host.hpp"
#include "simgrid/s4u/storage.hpp"
#include "../msg/msg_private.hpp"

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
}

Host::~Host() {
  delete pimpl_cpu;
  delete pimpl_netcard;
  delete mounts;
}

Host *Host::by_name(std::string name) {
  Host* host = Host::by_name_or_null(name.c_str());
  // TODO, raise an exception instead?
  if (host == nullptr)
    xbt_die("No such host: %s", name.c_str());
  return host;
}
Host* Host::by_name_or_null(const char* name)
{
  return (Host*) xbt_dict_get_or_null(host_list, name);
}
Host* Host::by_name_or_create(const char* name)
{
  Host* host = by_name_or_null(name);
  if (host == nullptr) {
    host = new Host(name);
    xbt_dict_set(host_list, name, host, nullptr);
  }
  return host;
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
      this->extension<simgrid::surf::HostImpl>()->turnOn();
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
  return simgrid::simix::kernelImmediate([&] {
    simgrid::surf::HostImpl* surf_host = this->extension<simgrid::surf::HostImpl>();
    return surf_host->getProperties();
  });
}

/** Retrieve the property value (or nullptr if not set) */
const char*Host::property(const char*key) {
  simgrid::surf::HostImpl* surf_host = this->extension<simgrid::surf::HostImpl>();
  return surf_host->getProperty(key);
}
void Host::setProperty(const char*key, const char *value){
  simgrid::simix::kernelImmediate([&] {
    simgrid::surf::HostImpl* surf_host = this->extension<simgrid::surf::HostImpl>();
    surf_host->setProperty(key,value);
  });
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
int Host::coresCount() {
  return pimpl_cpu->getCore();
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

void Host::parameters(vm_params_t params)
{
  simgrid::simix::kernelImmediate([&]() {
    this->extension<simgrid::surf::HostImpl>()->getParams(params);
  });
}

void Host::setParameters(vm_params_t params)
{
  simgrid::simix::kernelImmediate([&]() {
    this->extension<simgrid::surf::HostImpl>()->setParams(params);
  });
}

/**
 * \ingroup simix_storage_management
 * \brief Returns the list of storages mounted on an host.
 * \return a dict containing all storages mounted on the host
 */
xbt_dict_t Host::mountedStoragesAsDict()
{
  return simgrid::simix::kernelImmediate([&] {
    return this->extension<simgrid::surf::HostImpl>()->getMountedStorageList();
  });
}

/**
 * \ingroup simix_storage_management
 * \brief Returns the list of storages attached to an host.
 * \return a dict containing all storages attached to the host
 */
xbt_dynar_t Host::attachedStorages()
{
  return simgrid::simix::kernelImmediate([&] {
    return this->extension<simgrid::surf::HostImpl>()->getAttachedStorageList();
  });
}

} // namespace simgrid
} // namespace s4u
