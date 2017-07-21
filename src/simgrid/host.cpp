/* Copyright (c) 2013-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <algorithm>
#include <vector>

#include "simgrid/host.h"
#include "simgrid/s4u/Host.hpp"
#include "xbt/Extendable.hpp"
#include "xbt/dict.h"

#include "src/kernel/routing/NetPoint.hpp"
#include "src/simix/smx_host_private.h"
#include "src/surf/HostImpl.hpp"
#include "src/surf/cpu_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(sg_host, sd, "Logging specific to sg_hosts");

// FIXME: The following duplicates the content of s4u::Host
namespace simgrid {
namespace s4u {
extern std::map<std::string, simgrid::s4u::Host*> host_list;
}
}

extern "C" {

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
  for (auto kv : simgrid::s4u::host_list)
    names.push_back(kv.second->getName());

  std::sort(names.begin(), names.end());

  for (auto name : names)
    simgrid::s4u::host_list.at(name)->destroy();

  // host_list.clear(); This would be sufficient if the dict would contain smart_ptr. It's now useless
}

size_t sg_host_count()
{
  return simgrid::s4u::host_list.size();
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
  return host->getCname();
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
  return strcmp((*static_cast<simgrid::s4u::Host* const*>(pa))->getCname(),
                (*static_cast<simgrid::s4u::Host* const*>(pb))->getCname());
}

xbt_dynar_t sg_hosts_as_dynar()
{
  xbt_dynar_t res = xbt_dynar_new(sizeof(sg_host_t),nullptr);

  for (auto kv : simgrid::s4u::host_list) {
    simgrid::s4u::Host* host = kv.second;
    if (host && host->pimpl_netpoint && host->pimpl_netpoint->isHost())
      xbt_dynar_push(res, &host);
  }
  xbt_dynar_sort(res, hostcmp_voidp);
  return res;
}

xbt_dynar_t sg_host_get_processes_as_dynar(sg_host_t host)
{
  smx_actor_t process;

  xbt_dynar_t process_dynar = xbt_dynar_new(sizeof(smx_actor_t), nullptr);
  xbt_swag_t process_swag = host->extension<simgrid::simix::Host>()->process_list;
  
  xbt_swag_foreach(process, process_swag){
    xbt_dynar_push(process_dynar, &process);
  }
  
  return process_dynar;
}

int sg_host_get_process_count(sg_host_t host){
    return xbt_swag_size(host->extension<simgrid::simix::Host>()->process_list);
}

// ========= Layering madness ==============*

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

// ========= storage related functions ============
xbt_dict_t sg_host_get_mounted_storage_list(sg_host_t host){
  xbt_assert((host != nullptr), "Invalid parameters");
  xbt_dict_t res = xbt_dict_new_homogeneous(nullptr);
  for (auto elm : host->getMountedStorages()) {
    const char* mount_name = elm.first.c_str();
    sg_storage_t storage   = elm.second;
    xbt_dict_set(res, mount_name, (void*)storage->getName(), nullptr);
  }

  return res;
}

xbt_dynar_t sg_host_get_attached_storage_list(sg_host_t host){
  std::vector<const char*>* storage_vector = new std::vector<const char*>();
  xbt_dynar_t storage_dynar = xbt_dynar_new(sizeof(const char*), nullptr);
  host->getAttachedStorages(storage_vector);
  for (auto name : *storage_vector)
    xbt_dynar_push(storage_dynar, &name);
  delete storage_vector;
  return storage_dynar;
}

// =========== user-level functions ===============
// ================================================
/** @brief Returns the total speed of a host */
double sg_host_speed(sg_host_t host)
{
  return host->getSpeed();
}

double sg_host_get_available_speed(sg_host_t host)
{
  return host->pimpl_cpu->getAvailableSpeed();
}

/** @brief Returns the number of power states for a host.
 *
 *  See also @ref plugin_energy.
 */
int sg_host_get_nb_pstates(sg_host_t host) {
  return host->getPstatesCount();
}

/** @brief Gets the pstate at which that host currently runs.
 *
 *  See also @ref plugin_energy.
 */
int sg_host_get_pstate(sg_host_t host) {
  return host->getPstate();
}
/** @brief Sets the pstate at which that host should run.
 *
 *  See also @ref plugin_energy.
 */
void sg_host_set_pstate(sg_host_t host,int pstate) {
  host->setPstate(pstate);
}

/** @brief Get the properties of an host */
xbt_dict_t sg_host_get_properties(sg_host_t host) {
  return host->getProperties();
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
/**
 * \brief Find a route between two hosts
 *
 * \param from where from
 * \param to where to
 * \param links [OUT] where to store the list of links (must exist, cannot be nullptr).
 */
void sg_host_route(sg_host_t from, sg_host_t to, xbt_dynar_t links)
{
  std::vector<simgrid::s4u::Link*> vlinks;
  from->routeTo(to, &vlinks, nullptr);
  for (auto link : vlinks)
    xbt_dynar_push(links, &link);
}
/**
 * \brief Find the latency of the route between two hosts
 *
 * \param from where from
 * \param to where to
 */
double sg_host_route_latency(sg_host_t from, sg_host_t to)
{
  std::vector<simgrid::s4u::Link*> vlinks;
  double res = 0;
  from->routeTo(to, &vlinks, &res);
  return res;
}
/**
 * \brief Find the bandwitdh of the route between two hosts
 *
 * \param from where from
 * \param to where to
 */
double sg_host_route_bandwidth(sg_host_t from, sg_host_t to)
{
  double min_bandwidth = -1.0;

  std::vector<simgrid::s4u::Link*> vlinks;
  from->routeTo(to, &vlinks, nullptr);
  for (auto link : vlinks) {
    double bandwidth = link->bandwidth();
    if (bandwidth < min_bandwidth || min_bandwidth < 0.0)
      min_bandwidth = bandwidth;
  }
  return min_bandwidth;
}

/** @brief Displays debugging information about a host */
void sg_host_dump(sg_host_t host)
{
  xbt_dict_t props;

  XBT_INFO("Displaying host %s", host->getCname());
  XBT_INFO("  - speed: %.0f", host->getSpeed());
  XBT_INFO("  - available speed: %.2f", sg_host_get_available_speed(host));
  props = host->getProperties();

  if (not xbt_dict_is_empty(props)) {
    XBT_INFO("  - properties:");
    xbt_dict_cursor_t cursor = nullptr;
    char* key;
    char* data;

    xbt_dict_foreach(props,cursor,key,data) {
      XBT_INFO("    %s->%s",key,data);
    }
  }
}

} // extern "C"
