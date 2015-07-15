/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/dict.h"
#include "simgrid/host.h"
#include "surf/surf_routing.h" // SIMIX_HOST_LEVEL and friends FIXME: make private here

sg_host_t sg_host_by_name(const char *name){
  return xbt_lib_get_elm_or_null(host_lib, name);
}

// ========= Layering madness ==============

int SIMIX_HOST_LEVEL;
int MSG_HOST_LEVEL;
int SURF_CPU_LEVEL;

#include "simix/smx_host_private.h" // SIMIX_host_destroy. FIXME: killme
#include "msg/msg_private.h" // MSG_host_priv_free. FIXME: killme
#include "surf/cpu_interface.hpp"

static XBT_INLINE void surf_cpu_free(void *r) {
  delete static_cast<CpuPtr>(r);
}


void sg_host_init() {
  SIMIX_HOST_LEVEL = xbt_lib_add_level(host_lib,SIMIX_host_destroy);
  MSG_HOST_LEVEL = xbt_lib_add_level(host_lib, (void_f_pvoid_t) __MSG_host_priv_free);
  SURF_CPU_LEVEL = xbt_lib_add_level(host_lib,surf_cpu_free);
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



/*
host::host() {
	// TODO Auto-generated constructor stub

}

host::~host() {
	// TODO Auto-generated destructor stub
}*/

