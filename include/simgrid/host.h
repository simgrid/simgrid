/* Copyright (c) 2013-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_HOST_H_
#define SIMGRID_HOST_H_

#include <xbt/dict.h>

SG_BEGIN_DECL()

typedef xbt_dictelm_t sg_host_t;

typedef struct s_smx_host_priv *smx_host_priv_t;

// Function returning the SIMIX's private part of the host
XBT_PUBLIC(smx_host_priv_t) sg_host_simix(sg_host_t host);
XBT_PUBLIC(void) sg_host_simix_set(sg_host_t host, smx_host_priv_t priv);
XBT_PUBLIC(void) sg_host_simix_destroy(sg_host_t host);

// Module initializer. Won't survive the conversion to C++
XBT_PUBLIC(void) sg_host_init(void);

SG_END_DECL()

#endif /* SIMGRID_HOST_H_ */
