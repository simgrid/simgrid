/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/dict.h"
#include "simgrid/host.h"
#include "simgrid/Host.hpp"
#include "surf/surf.h" // routing_get_network_element_type FIXME:killme

size_t sg_host_count()
{
  return xbt_dict_length(host_list);
}

void* sg_host_get_facet(sg_host_t host, size_t facet)
{
  return host->facet(facet);
}

const char *sg_host_get_name(sg_host_t host)
{
	return host->id().c_str();
}

size_t sg_host_add_level(void(*deleter)(void*))
{
  return simgrid::Host::add_level(deleter);
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
int SURF_CPU_LEVEL;
int USER_HOST_LEVEL;

#include "src/msg/msg_private.h" // MSG_host_priv_free. FIXME: killme
#include "src/simdag/private.h" // __SD_workstation_destroy. FIXME: killme
#include "src/simix/smx_host_private.h" // SIMIX_host_destroy. FIXME: killme
#include "src/surf/cpu_interface.hpp"
#include "src/surf/surf_routing.hpp"

static XBT_INLINE void surf_cpu_free(void *r) {
  delete static_cast<simgrid::surf::Cpu*>(r);
}
static XBT_INLINE void routing_asr_host_free(void *p) {
  delete static_cast<simgrid::surf::RoutingEdge*>(p);
}

void sg_host_init()
{
  MSG_HOST_LEVEL = simgrid::Host::add_level([](void *p) {
    __MSG_host_priv_free((msg_host_priv_t) p);
  });
  SD_HOST_LEVEL = simgrid::Host::add_level(__SD_workstation_destroy);
  SIMIX_HOST_LEVEL = simgrid::Host::add_level(SIMIX_host_destroy);
  SURF_CPU_LEVEL = simgrid::Host::add_level(surf_cpu_free);
  ROUTING_HOST_LEVEL = simgrid::Host::add_level(routing_asr_host_free);
  USER_HOST_LEVEL = simgrid::Host::add_level(NULL);
}

// ========== User data Layer ==========
void *sg_host_user(sg_host_t host) {
  return host->facet(USER_HOST_LEVEL);
}
void sg_host_user_set(sg_host_t host, void* userdata) {
  host->set_facet(USER_HOST_LEVEL,userdata);
}
void sg_host_user_destroy(sg_host_t host) {
  host->set_facet(USER_HOST_LEVEL, nullptr);
}

// ========== MSG Layer ==============
msg_host_priv_t sg_host_msg(sg_host_t host) {
	return (msg_host_priv_t) host->facet(MSG_HOST_LEVEL);
}
void sg_host_msg_set(sg_host_t host, msg_host_priv_t smx_host) {
  host->set_facet(MSG_HOST_LEVEL, smx_host);
}
void sg_host_msg_destroy(sg_host_t host) {
  host->set_facet(MSG_HOST_LEVEL, nullptr);
}
// ========== SimDag Layer ==============
SD_workstation_priv_t sg_host_sd(sg_host_t host) {
  return (SD_workstation_priv_t) host->facet(SD_HOST_LEVEL);
}
void sg_host_sd_set(sg_host_t host, SD_workstation_priv_t smx_host) {
  host->set_facet(SD_HOST_LEVEL, smx_host);
}
void sg_host_sd_destroy(sg_host_t host) {
  host->set_facet(SD_HOST_LEVEL, nullptr);
}

// ========== Simix layer =============
smx_host_priv_t sg_host_simix(sg_host_t host){
  return (smx_host_priv_t) host->facet(SIMIX_HOST_LEVEL);
}
void sg_host_simix_set(sg_host_t host, smx_host_priv_t smx_host) {
  host->set_facet(SIMIX_HOST_LEVEL, smx_host);
}
void sg_host_simix_destroy(sg_host_t host) {
  host->set_facet(SIMIX_HOST_LEVEL, nullptr);
}

// ========== SURF CPU ============
surf_cpu_t sg_host_surfcpu(sg_host_t host) {
	return (surf_cpu_t) host->facet(SURF_CPU_LEVEL);
}
void sg_host_surfcpu_set(sg_host_t host, surf_cpu_t cpu) {
  host->set_facet(SURF_CPU_LEVEL, cpu);
}
void sg_host_surfcpu_register(sg_host_t host, surf_cpu_t cpu)
{
  surf_callback_emit(simgrid::surf::cpuCreatedCallbacks, cpu);
  surf_callback_emit(simgrid::surf::cpuStateChangedCallbacks, cpu, SURF_RESOURCE_ON, cpu->getState());
  sg_host_surfcpu_set(host, cpu);
}
void sg_host_surfcpu_destroy(sg_host_t host) {
  host->set_facet(SURF_CPU_LEVEL, nullptr);
}
// ========== RoutingEdge ============
surf_RoutingEdge *sg_host_edge(sg_host_t host) {
	return (surf_RoutingEdge*) host->facet(ROUTING_HOST_LEVEL);
}
void sg_host_edge_set(sg_host_t host, surf_RoutingEdge *edge) {
  host->set_facet(ROUTING_HOST_LEVEL, edge);
}
void sg_host_edge_destroy(sg_host_t host, int do_callback) {
  host->set_facet(ROUTING_HOST_LEVEL, nullptr, do_callback);
}

// =========== user-level functions ===============
// ================================================
double sg_host_get_speed(sg_host_t host){
  return surf_host_get_speed(host, 1.0);
}

double sg_host_get_available_speed(sg_host_t host){
  return surf_host_get_available_speed(host);
}
/** @brief Returns the number of core of the processor. */
int sg_host_get_core(sg_host_t host) {
	return surf_host_get_core(host);
}
/** @brief Returns the state of a host.
 *  @return 1 if the host is active or 0 if it has crashed.
 */
int sg_host_get_state(sg_host_t host) {
	return surf_host_get_state(surf_host_resource_priv(host));
}

/** @brief Returns the total energy consumed by the host (in Joules).
 *
 *  See also @ref SURF_plugin_energy.
 */
double sg_host_get_consumed_energy(sg_host_t host) {
	return surf_host_get_consumed_energy(host);
}

/** @brief Returns the number of power states for a host.
 *
 *  See also @ref SURF_plugin_energy.
 */
int sg_host_get_nb_pstates(sg_host_t host) {
	return surf_host_get_nb_pstates(host);
}

/** @brief Gets the pstate at which that host currently runs.
 *
 *  See also @ref SURF_plugin_energy.
 */
int sg_host_get_pstate(sg_host_t host) {
	return surf_host_get_pstate(host);
}

namespace simgrid {

Host::Host(std::string const& id)
  : id_(id)
{
}

Host::~Host()
{
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

}
