/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <string>
#include <functional>
#include <stdexcept>

#include <unordered_map>

#include <simgrid/simix.hpp>

#include "xbt/log.h"
#include "src/msg/msg_private.h"
#include "src/simix/smx_process_private.h"
#include "src/simix/smx_private.hpp"
#include "src/surf/cpu_interface.hpp"
#include "src/surf/host_interface.hpp"

#include "simgrid/s4u/host.hpp"
#include "simgrid/s4u/storage.hpp"

int MSG_HOST_LEVEL;
int SD_HOST_LEVEL;
int SIMIX_HOST_LEVEL;
int ROUTING_HOST_LEVEL;
int USER_HOST_LEVEL;

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
    xbt_dict_set(host_list, name, host, NULL);
  }
  return host;
}

Host *Host::current(){
  smx_process_t smx_proc = SIMIX_process_self();
  if (smx_proc == NULL)
    xbt_die("Cannot call Host::current() from the maestro context");
  return SIMIX_process_get_host(smx_proc);
}

void Host::turn_on() {
  simgrid::simix::kernel(std::bind(SIMIX_host_on, this));
}

void Host::turn_off() {
  simgrid::simix::kernel(std::bind(SIMIX_host_off, this, SIMIX_process_self()));
}

bool Host::is_on() {
  return this->pimpl_cpu->isOn();
}

int Host::pstates_count() const {
  return this->pimpl_cpu->getNbPStates();
}

boost::unordered_map<std::string, Storage*> const& Host::mounted_storages() {
  if (mounts == NULL) {
    mounts = new boost::unordered_map<std::string, Storage*> ();

    xbt_dict_t dict = this->mounted_storages_as_dict();

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
  return simgrid::simix::kernel([&] {
    simgrid::surf::Host* surf_host = this->extension<simgrid::surf::Host>();
    return surf_host->getProperties();
  });
}

/** Get the processes attached to the host */
xbt_swag_t Host::processes()
{
  return simgrid::simix::kernel([&]() {
    return ((smx_host_priv_t)this->extension(SIMIX_HOST_LEVEL))->process_list;
  });
}

/** Get the peak power of a host */
double Host::current_power_peak()
{
  return simgrid::simix::kernel([&] {
    return this->pimpl_cpu->getCurrentPowerPeak();
  });
}

/** Get one power peak (in flops/s) of a host at a given pstate */
double Host::power_peak_at(int pstate_index)
{
  return simgrid::simix::kernel([&] {
    return this->pimpl_cpu->getPowerPeakAt(pstate_index);
  });
}

/** @brief Get the speed of the cpu associated to a host */
double Host::speed() {
  return pimpl_cpu->getSpeed(1.0);
}
/** @brief Returns the number of core of the processor. */
int Host::core_count() {
  return pimpl_cpu->getCore();
}

/** @brief Set the pstate at which the host should run */
void Host::set_pstate(int pstate_index)
{
  simgrid::simix::kernel(std::bind(
      &simgrid::surf::Cpu::setPState, pimpl_cpu, pstate_index
  ));
}
/** @brief Retrieve the pstate at which the host is currently running */
int Host::pstate()
{
  return pimpl_cpu->getPState();
}

void Host::get_parameters(vm_params_t params)
{
  simgrid::simix::kernel([&]() {
    this->extension<simgrid::surf::Host>()->getParams(params);
  });
}

void Host::set_parameters(vm_params_t params)
{
  simgrid::simix::kernel([&]() {
    this->extension<simgrid::surf::Host>()->setParams(params);
  });
}

/**
 * \ingroup simix_storage_management
 * \brief Returns the list of storages mounted on an host.
 * \return a dict containing all storages mounted on the host
 */
xbt_dict_t Host::mounted_storages_as_dict()
{
  return simgrid::simix::kernel([&] {
    return this->extension<simgrid::surf::Host>()->getMountedStorageList();
  });
}

/**
 * \ingroup simix_storage_management
 * \brief Returns the list of storages attached to an host.
 * \return a dict containing all storages attached to the host
 */
xbt_dynar_t Host::attached_storages()
{
  return simgrid::simix::kernel([&] {
    return this->extension<simgrid::surf::Host>()->getAttachedStorageList();
  });
}

} // namespace simgrid
} // namespace s4u
