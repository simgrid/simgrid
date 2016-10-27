/* Copyright (c) 2013-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <vector>

#include "xbt/dict.h"
#include "simgrid/host.h"
#include <xbt/Extendable.hpp>
#include <simgrid/s4u/host.hpp>

#include "src/surf/HostImpl.hpp"
#include "surf/surf.h" // routing_get_network_element_type FIXME:killme

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sg_host, sd, "Logging specific to sg_hosts");

extern std::unordered_map<simgrid::xbt::string, simgrid::s4u::Host*>
    host_list; // FIXME: don't dupplicate the content of s4u::Host this way

void sg_host_exit()
{
  /* copy all names to not modify the map while iterating over it.
   *
   * Plus, the hosts are destroyed in the lexicographic order to ensure
   * that the output is reproducible: we don't want to kill them in the
   * pointer order as it could be platform-dependent, which would break
   * the tests.
   */
  std::vector<std::string> names = std::vector<std::string>();
  for (auto kv : host_list)
    names.push_back(kv.second->name());

  std::sort(names.begin(), names.end());

  for (auto name : names)
    host_list.at(name)->destroy();

  // host_list.clear(); This would be sufficient if the dict would contain smart_ptr. It's now useless
}

size_t sg_host_count()
{
  return host_list.size();
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
sg_host_t *sg_host_list() {
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

static int hostcmp_voidp(const void* pa, const void* pb)
{
  return strcmp((*static_cast<simgrid::s4u::Host* const*>(pa))->name().c_str(),
                (*static_cast<simgrid::s4u::Host* const*>(pb))->name().c_str());
}

xbt_dynar_t sg_hosts_as_dynar()
{
  xbt_dynar_t res = xbt_dynar_new(sizeof(sg_host_t),nullptr);

  for (auto kv : host_list) {
    simgrid::s4u::Host* host = kv.second;
    if (host && host->pimpl_netcard && host->pimpl_netcard->isHost())
       xbt_dynar_push(res, &host);
  }
  xbt_dynar_sort(res, hostcmp_voidp);
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
#include "src/simix/smx_host_private.h"
smx_host_priv_t sg_host_simix(sg_host_t host){
  return host->extension<simgrid::simix::Host>();
}

// ========= storage related functions ============
xbt_dict_t sg_host_get_mounted_storage_list(sg_host_t host){
  return host->pimpl_->getMountedStorageList();
}

xbt_dynar_t sg_host_get_attached_storage_list(sg_host_t host){
  return host->pimpl_->getAttachedStorageList();
}


// =========== user-level functions ===============
// ================================================
/** @brief Returns the total speed of a host */
double sg_host_speed(sg_host_t host)
{
  return host->speed();
}

double sg_host_get_available_speed(sg_host_t host)
{
  return host->pimpl_cpu->getAvailableSpeed();
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
 * \return value of a property (or nullptr if property not set)
*/
const char *sg_host_get_property_value(sg_host_t host, const char *name)
{
  return (const char*) xbt_dict_get_or_null(sg_host_get_properties(host), name);
}

/** @brief Displays debugging information about a host */
void sg_host_dump(sg_host_t host)
{
  xbt_dict_t props;
  xbt_dict_cursor_t cursor=nullptr;
  char *key,*data;

  XBT_INFO("Displaying host %s", sg_host_get_name(host));
  XBT_INFO("  - speed: %.0f", host->speed());
  XBT_INFO("  - available speed: %.2f", sg_host_get_available_speed(host));
  props = sg_host_get_properties(host);

  if (!xbt_dict_is_empty(props)){
    XBT_INFO("  - properties:");

    xbt_dict_foreach(props,cursor,key,data) {
      XBT_INFO("    %s->%s",key,data);
    }
  }
}
