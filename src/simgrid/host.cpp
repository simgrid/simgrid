/* Copyright (c) 2013-201. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/dict.h"
#include "simgrid/host.h"
#include <xbt/Extendable.hpp>
#include <simgrid/s4u/host.hpp>
#include "surf/surf.h" // routing_get_network_element_type FIXME:killme

#include "src/simix/smx_private.hpp"
#include "src/surf/host_interface.hpp"

size_t sg_host_count()
{
  return xbt_dict_length(host_list);
}

const char *sg_host_get_name(sg_host_t host)
{
	return host->name().c_str();
}

void* sg_host_extension_get(sg_host_t host, size_t ext)
{
  return host->extension(ext);
}

size_t sg_host_extension_create(void(*deleter)(void*))
{
  return simgrid::s4u::Host::extension_create(deleter);
}

sg_host_t sg_host_by_name(const char *name)
{
  return simgrid::s4u::Host::by_name_or_null(name);
}

sg_host_t sg_host_by_name_or_create(const char *name)
{
  return simgrid::s4u::Host::by_name_or_create(name);
}

xbt_dynar_t sg_hosts_as_dynar(void)
{
	xbt_dynar_t res = xbt_dynar_new(sizeof(sg_host_t),NULL);

  xbt_dict_cursor_t cursor = nullptr;
  const char* name = nullptr;
  simgrid::s4u::Host* host = nullptr;
	xbt_dict_foreach(host_list, cursor, name, host)
		if(routing_get_network_element_type(name) == SURF_NETWORK_ELEMENT_HOST)
			xbt_dynar_push(res, &host);
	return res;
}

// ========= Layering madness ==============*

#include "src/msg/msg_private.h" // MSG_host_priv_free. FIXME: killme by initializing that level in msg when used
#include "src/simix/smx_host_private.h" // SIMIX_host_destroy. FIXME: killme by initializing that level in simix when used
#include "src/surf/cpu_interface.hpp"
#include "src/surf/surf_routing.hpp"

void sg_host_init()
{
  MSG_HOST_LEVEL = simgrid::s4u::Host::extension_create([](void *p) {
    __MSG_host_priv_free((msg_host_priv_t) p);
  });

  ROUTING_HOST_LEVEL = simgrid::s4u::Host::extension_create([](void *p) {
	  delete static_cast<simgrid::surf::NetCard*>(p);
  });

  SD_HOST_LEVEL = simgrid::s4u::Host::extension_create(NULL);
  SIMIX_HOST_LEVEL = simgrid::s4u::Host::extension_create(SIMIX_host_destroy);
  USER_HOST_LEVEL = simgrid::s4u::Host::extension_create(NULL);
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
	return host->is_on();
}

/** @brief Returns the number of power states for a host.
 *
 *  See also @ref SURF_plugin_energy.
 */
int sg_host_get_nb_pstates(sg_host_t host) {
  return host->pstates_count();
}

/** @brief Gets the pstate at which that host currently runs.
 *
 *  See also @ref SURF_plugin_energy.
 */
int sg_host_get_pstate(sg_host_t host) {
  return host->pstate();
}
/** @brief Sets the pstate at which that host should run.
 *
 *  See also @ref SURF_plugin_energy.
 */
void sg_host_set_pstate(sg_host_t host,int pstate) {
  host->set_pstate(pstate);
}

/** @brief Get the properties of an host */
xbt_dict_t sg_host_get_properties(sg_host_t host) {
  return host->properties();
}


/** \ingroup m_host_management
 * \brief Returns the value of a given host property
 *
 * \param host a host
 * \param name a property name
 * \return value of a property (or NULL if property not set)
*/
const char *sg_host_get_property_value(sg_host_t host, const char *name)
{
  return (const char*) xbt_dict_get_or_null(sg_host_get_properties(host), name);
}

