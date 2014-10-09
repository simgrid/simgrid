/* MC interface: definitions that non-MC modules must see, but not the user */

/* Copyright (c) 2007-2014. The SimGrid Team.  All rights reserved.         */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_INTERFACE_H
#define MC_INTERFACE_H
#include "mc/mc.h"

SG_BEGIN_DECL()

typedef struct s_mc_snapshot *mc_snapshot_t;

/* These are the MC-specific simcalls, that smx_user needs to see */
mc_snapshot_t SIMIX_pre_mc_snapshot(smx_simcall_t simcall);
int SIMIX_pre_mc_compare_snapshots(smx_simcall_t simcall, mc_snapshot_t s1, mc_snapshot_t s2);
int SIMIX_pre_mc_random(smx_simcall_t simcall, int min, int max);

SG_END_DECL()

#endif
