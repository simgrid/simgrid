/* Copyright (c) 2013-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_HOST_H_
#define SIMGRID_HOST_H_

#include <stddef.h>

#include <xbt/dict.h>
#include <xbt/dynar.h>

#include <simgrid/forward.h>

SG_BEGIN_DECL()

XBT_PUBLIC(size_t) sg_host_count(void);
XBT_PUBLIC(sg_host_t *) sg_host_list(void);

XBT_PUBLIC(size_t) sg_host_extension_create(void(*deleter)(void*));
XBT_PUBLIC(void*) sg_host_extension_get(sg_host_t host, size_t rank);
XBT_PUBLIC(sg_host_t) sg_host_by_name(const char *name);
XBT_PUBLIC(const char*) sg_host_get_name(sg_host_t host);
XBT_PUBLIC(xbt_dynar_t) sg_hosts_as_dynar(void);

// ========== User Data ==============
XBT_PUBLIC(void*) sg_host_user(sg_host_t host);
XBT_PUBLIC(void) sg_host_user_set(sg_host_t host, void* userdata);
XBT_PUBLIC(void) sg_host_user_destroy(sg_host_t host);

// ========== MSG Layer ==============
typedef struct s_msg_host_priv *msg_host_priv_t;
msg_host_priv_t sg_host_msg(sg_host_t host);
XBT_PUBLIC(void) sg_host_msg_set(sg_host_t host, msg_host_priv_t priv);

// ========== Simix layer =============
typedef struct s_smx_host_priv *smx_host_priv_t;
XBT_PUBLIC(smx_host_priv_t) sg_host_simix(sg_host_t host);
XBT_PUBLIC(void) sg_host_simix_set(sg_host_t host, smx_host_priv_t priv);
XBT_PUBLIC(void) sg_host_simix_destroy(sg_host_t host);

// ========= storage related functions ============
XBT_PUBLIC(xbt_dict_t) sg_host_get_mounted_storage_list(sg_host_t host);
XBT_PUBLIC(xbt_dynar_t) sg_host_get_attached_storage_list(sg_host_t host);
// =========== user-level functions ===============
XBT_PUBLIC(double) sg_host_speed(sg_host_t host);
XBT_PUBLIC(double) sg_host_get_available_speed(sg_host_t host);

XBT_PUBLIC(int) sg_host_get_nb_pstates(sg_host_t host);
XBT_PUBLIC(int) sg_host_get_pstate(sg_host_t host);
XBT_PUBLIC(void) sg_host_set_pstate(sg_host_t host,int pstate);
XBT_PUBLIC(xbt_dict_t) sg_host_get_properties(sg_host_t host);
XBT_PUBLIC(const char *) sg_host_get_property_value(sg_host_t host,
                                                     const char *name);
XBT_PUBLIC(void) sg_host_dump(sg_host_t ws);
SG_END_DECL()

#endif /* SIMGRID_HOST_H_ */
