/* Copyright (c) 2013-201. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/simix.hpp>

#include "xbt/dict.h"
#include "simgrid/host.h"
#include "simgrid/Host.hpp"
#include "surf/surf.h" // routing_get_network_element_type FIXME:killme

#include "src/simix/smx_private.hpp"
#include "src/surf/host_interface.hpp"

size_t sg_host_count()
{
  return xbt_dict_length(host_list);
}

const char *sg_host_get_name(sg_host_t host)
{
	return host->getName().c_str();
}

void* sg_host_extension_get(sg_host_t host, size_t ext)
{
  return host->extension(ext);
}

size_t sg_host_extension_create(void(*deleter)(void*))
{
  return simgrid::Host::extension_create(deleter);
}

sg_host_t sg_host_by_name(const char *name)
{
  return simgrid::Host::by_name_or_null(name);
}

sg_host_t sg_host_by_name_or_create(const char *name)
{
  return simgrid::Host::by_name_or_create(name);
}

xbt_dynar_t sg_hosts_as_dynar(void)
{
	xbt_dynar_t res = xbt_dynar_new(sizeof(sg_host_t),NULL);

  xbt_dict_cursor_t cursor = nullptr;
  const char* name = nullptr;
  simgrid::Host* host = nullptr;
	xbt_dict_foreach(host_list, cursor, name, host)
		if(routing_get_network_element_type(name) == SURF_NETWORK_ELEMENT_HOST)
			xbt_dynar_push(res, &host);
	return res;
}

// ========= Layering madness ==============

int MSG_HOST_LEVEL;
int SD_HOST_LEVEL;
int SIMIX_HOST_LEVEL;
int ROUTING_HOST_LEVEL;
int USER_HOST_LEVEL;

#include "src/msg/msg_private.h" // MSG_host_priv_free. FIXME: killme by initializing that level in msg when used
#include "src/simdag/simdag_private.h" // __SD_workstation_destroy. FIXME: killme by initializing that level in simdag when used
#include "src/simix/smx_host_private.h" // SIMIX_host_destroy. FIXME: killme by initializing that level in simix when used
#include "src/surf/cpu_interface.hpp"
#include "src/surf/surf_routing.hpp"

void sg_host_init()
{
  MSG_HOST_LEVEL = simgrid::Host::extension_create([](void *p) {
    __MSG_host_priv_free((msg_host_priv_t) p);
  });

  ROUTING_HOST_LEVEL = simgrid::Host::extension_create([](void *p) {
	  delete static_cast<simgrid::surf::NetCard*>(p);
  });

  SD_HOST_LEVEL = simgrid::Host::extension_create(__SD_workstation_destroy);
  SIMIX_HOST_LEVEL = simgrid::Host::extension_create(SIMIX_host_destroy);
  USER_HOST_LEVEL = simgrid::Host::extension_create(NULL);
}

// ========== User data Layer ==========
void *sg_host_user(sg_host_t host) {
  return host->extension(USER_HOST_LEVEL);
}
void sg_host_user_set(sg_host_t host, void* userdata) {
  host->extension_set(USER_HOST_LEVEL,userdata);
}
void sg_host_user_destroy(sg_host_t host) {
  host->extension_set(USER_HOST_LEVEL, nullptr);
}

// ========== MSG Layer ==============
msg_host_priv_t sg_host_msg(sg_host_t host) {
	return (msg_host_priv_t) host->extension(MSG_HOST_LEVEL);
}
void sg_host_msg_set(sg_host_t host, msg_host_priv_t smx_host) {
  host->extension_set(MSG_HOST_LEVEL, smx_host);
}
// ========== SimDag Layer ==============
SD_workstation_priv_t sg_host_sd(sg_host_t host) {
  return (SD_workstation_priv_t) host->extension(SD_HOST_LEVEL);
}
void sg_host_sd_set(sg_host_t host, SD_workstation_priv_t smx_host) {
  host->extension_set(SD_HOST_LEVEL, smx_host);
}
void sg_host_sd_destroy(sg_host_t host) {
  host->extension_set(SD_HOST_LEVEL, nullptr);
}

// ========== Simix layer =============
smx_host_priv_t sg_host_simix(sg_host_t host){
  return (smx_host_priv_t) host->extension(SIMIX_HOST_LEVEL);
}
void sg_host_simix_set(sg_host_t host, smx_host_priv_t smx_host) {
  host->extension_set(SIMIX_HOST_LEVEL, smx_host);
}
void sg_host_simix_destroy(sg_host_t host) {
  host->extension_set(SIMIX_HOST_LEVEL, nullptr);
}

// =========== user-level functions ===============
// ================================================

double sg_host_get_available_speed(sg_host_t host){
  return surf_host_get_available_speed(host);
}
/** @brief Returns the state of a host.
 *  @return 1 if the host is active or 0 if it has crashed.
 */
int sg_host_is_on(sg_host_t host) {
	return host->pimpl_cpu->isOn();
}

/** @brief Returns the number of power states for a host.
 *
 *  See also @ref SURF_plugin_energy.
 */
int sg_host_get_nb_pstates(sg_host_t host) {
  return host->pimpl_cpu->getNbPStates();
}

/** @brief Gets the pstate at which that host currently runs.
 *
 *  See also @ref SURF_plugin_energy.
 */
int sg_host_get_pstate(sg_host_t host) {
  return host->getPState();
}
/** @brief Sets the pstate at which that host should run.
 *
 *  See also @ref SURF_plugin_energy.
 */
void sg_host_set_pstate(sg_host_t host,int pstate) {
  host->setPState(pstate);
}

/** @brief Get the properties of an host */
xbt_dict_t sg_host_get_properties(sg_host_t host) {
  return host->getProperties();
}


namespace simgrid {

Host::Host(std::string const& id)
  : name_(id)
{
}

Host::~Host()
{
}

/** Start the host if it is off */
void Host::turnOn()
{
  simgrid::simix::kernel(std::bind(SIMIX_host_on, this));
}

/** Stop the host if it is on */
void Host::turnOff()
{
  simgrid::simix::simcall<void>(SIMCALL_HOST_OFF, this);
}

bool Host::isOn() {
  return pimpl_cpu->isOn();
}
bool Host::isOff() {
  return ! pimpl_cpu->isOn();
}


/** Get the properties assigned to a host */
xbt_dict_t Host::getProperties()
{
  return simgrid::simix::kernel(std::bind(&simgrid::surf::Host::getProperties, this->extension(simgrid::surf::Host::EXTENSION_ID)));
}

/** Get the processes attached to the host */
xbt_swag_t Host::getProcessList()
{
  return simgrid::simix::kernel([&]() {
    return ((smx_host_priv_t)this->extension(SIMIX_HOST_LEVEL))->process_list;
  });
}

/** Get the peak power of a host */
double Host::getCurrentPowerPeak()
{
  return simgrid::simix::kernel([&] {
    return this->pimpl_cpu->getCurrentPowerPeak();
  });
}

/** Get one power peak (in flops/s) of a host at a given pstate */
double Host::getPowerPeakAt(int pstate_index)
{
  return simgrid::simix::kernel([&] {
    return this->pimpl_cpu->getPowerPeakAt(pstate_index);
  });
}

/** @brief Get the speed of the cpu associated to a host */
double Host::getSpeed() {
	return pimpl_cpu->getSpeed(1.0);
}
/** @brief Returns the number of core of the processor. */
int Host::getCoreAmount() {
	return pimpl_cpu->getCore();
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

/** @brief Set the pstate at which the host should run */
void Host::setPState(int pstate_index)
{
  simgrid::simix::kernel(std::bind(
      &simgrid::surf::Cpu::setPState, pimpl_cpu, pstate_index
  ));
}
/** @brief Retrieve the pstate at which the host is currently running */
int Host::getPState()
{
  return pimpl_cpu->getPState();
}

void Host::getParams(vm_params_t params)
{
  simgrid::simix::kernel([&]() {
    this->extension<simgrid::surf::Host>()->getParams(params);
  });
}

void Host::setParams(vm_params_t params)
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
xbt_dict_t Host::getMountedStorageList()
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
xbt_dynar_t Host::getAttachedStorageList()
{
  return simgrid::simix::kernel([&] {
    return this->extension<simgrid::surf::Host>()->getAttachedStorageList();
  });
}

}
