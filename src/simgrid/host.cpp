/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/dict.h"
#include "simgrid/host.h"
#include "surf/surf_routing.h" // SIMIX_HOST_LEVEL and friends FIXME: make private here

int SIMIX_HOST_LEVEL;           //Simix host level

#include "simix/smx_host_private.h" // SIMIX_host_destroy. FIXME: killme
void sg_host_init() {
	SIMIX_HOST_LEVEL = xbt_lib_add_level(host_lib,SIMIX_host_destroy);
}

smx_host_priv_t sg_host_simix(sg_host_t host){
  return (smx_host_priv_t) xbt_lib_get_level(host, SIMIX_HOST_LEVEL);
}
void sg_host_simix_set(sg_host_t host, smx_host_priv_t smx_host) {
	  xbt_lib_set(host_lib,host->key,SIMIX_HOST_LEVEL,smx_host);
}
void sg_host_simix_destroy(sg_host_t host) {
	  xbt_lib_unset(host_lib,host->key,SIMIX_HOST_LEVEL,1);
}


/*
host::host() {
	// TODO Auto-generated constructor stub

}

host::~host() {
	// TODO Auto-generated destructor stub
}*/

