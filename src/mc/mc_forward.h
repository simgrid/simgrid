/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/** \file mc_forward.h
 *
 *  Define type names for pointers of MC objects for the C code
 */

#ifndef SIMGRID_MC_FORWARD_H
#define SIMGRID_MC_FORWARD_H

#ifdef __cplusplus

// If we're in C++, we give the real definition:
#include "mc_forward.hpp"
typedef simgrid::mc::Snapshot *mc_snapshot_t;
typedef simgrid::mc::Type *mc_type_t;

#else

// Otherwise we use dummy opaque structs:
typedef struct _mc_snapshot_t *mc_snapshot_t;
typedef struct _s_mc_type_t *mc_type_t;

#endif

#endif
