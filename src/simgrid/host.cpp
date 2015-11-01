/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/dict.h"
#include "simgrid/host.h"
#include "surf/surf_routing.h" // SIMIX_HOST_LEVEL and friends FIXME: make private here
#include "surf/surf.h" // routing_get_network_element_type FIXME:killme

sg_host_t sg_host_by_name(const char *name){
  return xbt_lib_get_elm_or_null(host_lib, name);
}

sg_host_t sg_host_by_name_or_create(const char *name) {
	sg_host_t res = xbt_lib_get_elm_or_null(host_lib, name);
	if (!res) {
		xbt_lib_set(host_lib,name,0,NULL); // Should only create the bucklet with no data added
		res = xbt_lib_get_elm_or_null(host_lib, name);
	}
	return res;
}
xbt_dynar_t sg_hosts_as_dynar(void) {
	xbt_lib_cursor_t cursor;
	char *key;
	void **data;
	xbt_dynar_t res = xbt_dynar_new(sizeof(sg_host_t),NULL);

	xbt_lib_foreach(host_lib, cursor, key, data) {
		if(routing_get_network_element_type(key) == SURF_NETWORK_ELEMENT_HOST) {
			xbt_dictelm_t elm = xbt_dict_cursor_get_elm(cursor);
			xbt_dynar_push(res, &elm);
		}
	}
	return res;
}

// ========= Layering madness ==============

int MSG_HOST_LEVEL;
int SD_HOST_LEVEL;
int SIMIX_HOST_LEVEL;
int ROUTING_HOST_LEVEL;
int SURF_CPU_LEVEL;


#include "src/msg/msg_private.h" // MSG_host_priv_free. FIXME: killme
#include "src/simdag/private.h" // __SD_workstation_destroy. FIXME: killme
#include "src/simix/smx_host_private.h" // SIMIX_host_destroy. FIXME: killme
#include "src/surf/cpu_interface.hpp"
#include "src/surf/surf_routing.hpp"

static XBT_INLINE void surf_cpu_free(void *r) {
  delete static_cast<Cpu*>(r);
}
static XBT_INLINE void routing_asr_host_free(void *p) {
  delete static_cast<RoutingEdge*>(p);
}


void sg_host_init() { // FIXME: only add the needed levels
  MSG_HOST_LEVEL = xbt_lib_add_level(host_lib, (void_f_pvoid_t) __MSG_host_priv_free);
  SD_HOST_LEVEL = xbt_lib_add_level(host_lib,__SD_workstation_destroy);

  SIMIX_HOST_LEVEL = xbt_lib_add_level(host_lib,SIMIX_host_destroy);
  SURF_CPU_LEVEL = xbt_lib_add_level(host_lib,surf_cpu_free);
  ROUTING_HOST_LEVEL = xbt_lib_add_level(host_lib,routing_asr_host_free);
}

// ========== MSG Layer ==============
msg_host_priv_t sg_host_msg(sg_host_t host) {
	return (msg_host_priv_t) xbt_lib_get_level(host, MSG_HOST_LEVEL);
}
void sg_host_msg_set(sg_host_t host, msg_host_priv_t smx_host) {
	  xbt_lib_set(host_lib,host->key,MSG_HOST_LEVEL,smx_host);
}
void sg_host_msg_destroy(sg_host_t host) {
	  xbt_lib_unset(host_lib,host->key,MSG_HOST_LEVEL,1);
}
// ========== SimDag Layer ==============
SD_workstation_priv_t sg_host_sd(sg_host_t host) {
       return (SD_workstation_priv_t) xbt_lib_get_level(host, SD_HOST_LEVEL);
}
void sg_host_sd_set(sg_host_t host, SD_workstation_priv_t smx_host) {
         xbt_lib_set(host_lib,host->key,SD_HOST_LEVEL,smx_host);
}
void sg_host_sd_destroy(sg_host_t host) {
         xbt_lib_unset(host_lib,host->key,SD_HOST_LEVEL,1);
}

// ========== Simix layer =============
smx_host_priv_t sg_host_simix(sg_host_t host){
  return (smx_host_priv_t) xbt_lib_get_level(host, SIMIX_HOST_LEVEL);
}
void sg_host_simix_set(sg_host_t host, smx_host_priv_t smx_host) {
	xbt_lib_set(host_lib,host->key,SIMIX_HOST_LEVEL,smx_host);
}
void sg_host_simix_destroy(sg_host_t host) {
	xbt_lib_unset(host_lib,host->key,SIMIX_HOST_LEVEL,1);
}

// ========== SURF CPU ============
surf_cpu_t sg_host_surfcpu(sg_host_t host) {
	return (surf_cpu_t) xbt_lib_get_level(host, SURF_CPU_LEVEL);
}
void sg_host_surfcpu_set(sg_host_t host, surf_cpu_t cpu) {
	xbt_lib_set(host_lib, host->key, SURF_CPU_LEVEL, cpu);
}
void sg_host_surfcpu_destroy(sg_host_t host) {
	xbt_lib_unset(host_lib,host->key,SURF_CPU_LEVEL,1);
}
// ========== RoutingEdge ============
RoutingEdge *sg_host_edge(sg_host_t host) {
	return (RoutingEdge*) xbt_lib_get_level(host, ROUTING_HOST_LEVEL);
}
void sg_host_edge_set(sg_host_t host, RoutingEdge *edge) {
	xbt_lib_set(host_lib, host->key, ROUTING_HOST_LEVEL, edge);
}
void sg_host_edge_destroy(sg_host_t host, int do_callback) {
	xbt_lib_unset(host_lib,host->key,ROUTING_HOST_LEVEL,do_callback);
}


// =========== user-level functions ===============
// ================================================
double sg_host_get_speed(sg_host_t host){
  return surf_host_get_speed(host, 1.0);
}

double sg_host_get_available_speed(sg_host_t host){
  return surf_host_get_available_speed(host);
}
