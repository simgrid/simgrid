/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_HOST_H_
#define SIMGRID_HOST_H_

#include <xbt/dict.h>
#include <xbt/dynar.h>
SG_BEGIN_DECL()

typedef xbt_dictelm_t sg_host_t;
XBT_PUBLIC(sg_host_t) sg_host_by_name(const char *name);
XBT_PUBLIC(sg_host_t) sg_host_by_name_or_create(const char *name);
static XBT_INLINE char *sg_host_get_name(sg_host_t host){
	return host->key;
}
XBT_PUBLIC(xbt_dynar_t) sg_hosts_as_dynar(void);

#ifdef __cplusplus
#define DEFINE_EXTERNAL_CLASS(klass) class klass;
class Cpu;
#else
#define DEFINE_EXTERNAL_CLASS(klass) typedef struct klass klass;
#endif

// ========== MSG Layer ==============
typedef struct s_msg_host_priv *msg_host_priv_t;
msg_host_priv_t sg_host_msg(sg_host_t host);
XBT_PUBLIC(void) sg_host_msg_set(sg_host_t host, msg_host_priv_t priv);
XBT_PUBLIC(void) sg_host_msg_destroy(sg_host_t host);

// ========== SD Layer ==============
typedef struct SD_workstation *SD_workstation_priv_t;
SD_workstation_priv_t sg_host_sd(sg_host_t host);
XBT_PUBLIC(void) sg_host_sd_set(sg_host_t host, SD_workstation_priv_t priv);
XBT_PUBLIC(void) sg_host_sd_destroy(sg_host_t host);

// ========== Simix layer =============
typedef struct s_smx_host_priv *smx_host_priv_t;
XBT_PUBLIC(smx_host_priv_t) sg_host_simix(sg_host_t host);
XBT_PUBLIC(void) sg_host_simix_set(sg_host_t host, smx_host_priv_t priv);
XBT_PUBLIC(void) sg_host_simix_destroy(sg_host_t host);

// ========== SURF CPU ============
DEFINE_EXTERNAL_CLASS(Cpu);
typedef Cpu *surf_cpu_t;
XBT_PUBLIC(surf_cpu_t) sg_host_surfcpu(sg_host_t host);
XBT_PUBLIC(void) sg_host_surfcpu_set(sg_host_t host, surf_cpu_t cpu);
XBT_PUBLIC(void) sg_host_surfcpu_destroy(sg_host_t host);

// ========== RoutingEdge ============
DEFINE_EXTERNAL_CLASS(RoutingEdge);
XBT_PUBLIC(RoutingEdge*) sg_host_edge(sg_host_t host);
XBT_PUBLIC(void) sg_host_edge_set(sg_host_t host, RoutingEdge* edge);
XBT_PUBLIC(void) sg_host_edge_destroy(sg_host_t host, int do_callback);


// Module initializer. Won't survive the conversion to C++. Hopefully.
XBT_PUBLIC(void) sg_host_init(void);

// =========== user-level functions ===============
XBT_PUBLIC(double) sg_host_get_speed(sg_host_t host);
XBT_PUBLIC(double) sg_host_get_available_speed(sg_host_t host);


SG_END_DECL()

#endif /* SIMGRID_HOST_H_ */
