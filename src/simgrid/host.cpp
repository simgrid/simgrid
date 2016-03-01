/* Copyright (c) 2013-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/dict.h"
#include "simgrid/host.h"
#include <xbt/Extendable.hpp>
#include <simgrid/s4u/host.hpp>

#include "src/surf/HostImpl.hpp"
#include "surf/surf.h" // routing_get_network_element_type FIXME:killme

#include "src/simix/smx_private.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sg_host, sd, "Logging specific to sg_hosts");

size_t sg_host_count()
{
  return xbt_dict_length(host_list);
}
/** @brief Returns the host list
 *
 * Uses sg_host_count() to know the array size.
 *
 * \return an array of \ref sg_host_t containing all the hosts in the platform.
 * \remark The host order in this array is generally different from the
 * creation/declaration order in the XML platform (we use a hash table
 * internally).
 * \see sg_host_count()
 */
sg_host_t *sg_host_list(void) {
  xbt_assert(sg_host_count() > 0, "There is no host!");
  return (sg_host_t*)xbt_dynar_to_array(sg_hosts_as_dynar());
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
    if (host && host->pimpl_netcard && host->pimpl_netcard->isHost())
       xbt_dynar_push(res, &host);
  return res;
}

// ========= Layering madness ==============*

#include "src/surf/cpu_interface.hpp"
#include "src/surf/surf_routing.hpp"

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

// ========= storage related functions ============
xbt_dict_t sg_host_get_mounted_storage_list(sg_host_t host){
  return host->extension<simgrid::surf::HostImpl>()->getMountedStorageList();
}

xbt_dynar_t sg_host_get_attached_storage_list(sg_host_t host){
  return host->extension<simgrid::surf::HostImpl>()->getAttachedStorageList();
}


// =========== user-level functions ===============
// ================================================

/** @brief Returns the total speed of a host
 */
double sg_host_speed(sg_host_t host)
{
  return host->speed();
}

double sg_host_get_available_speed(sg_host_t host){
  return surf_host_get_available_speed(host);
}
/** @brief Returns the number of cores of a host
*/
int sg_host_core_count(sg_host_t host) {
  return host->core_count();
}

/** @brief Returns the state of a host.
 *  @return 1 if the host is active or 0 if it has crashed.
 */
int sg_host_is_on(sg_host_t host) {
  return host->isOn();
}

/** @brief Returns the number of power states for a host.
 *
 *  See also @ref SURF_plugin_energy.
 */
int sg_host_get_nb_pstates(sg_host_t host) {
  return host->pstatesCount();
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
  host->setPstate(pstate);
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

/** @brief Displays debugging informations about a host */
void sg_host_dump(sg_host_t host)
{
  xbt_dict_t props;
  xbt_dict_cursor_t cursor=NULL;
  char *key,*data;

  XBT_INFO("Displaying host %s", sg_host_get_name(host));
  XBT_INFO("  - speed: %.0f", sg_host_speed(host));
  XBT_INFO("  - available speed: %.2f", sg_host_get_available_speed(host));
  props = sg_host_get_properties(host);

  if (!xbt_dict_is_empty(props)){
    XBT_INFO("  - properties:");

    xbt_dict_foreach(props,cursor,key,data) {
      XBT_INFO("    %s->%s",key,data);
    }
  }
}
