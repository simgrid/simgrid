/* simgrid/modelchecker.h - Formal Verification made possible in SimGrid    */

/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MODELCHECKER_H
#define SIMGRID_MODELCHECKER_H

#include <stddef.h> /* size_t */
#include <xbt/base.h>

SG_BEGIN_DECL

/** Explore every branches where that function returns a value between min and max (inclusive) */
XBT_PUBLIC int MC_random(int min, int max);

/** Assertion for the model-checker: Defines a safety property to verify */
XBT_PUBLIC void MC_assert(int);

/** Check whether the model-checker is currently active, ie if this process was started with simgrid-mc.
 *  It is off in simulation or when replaying MC traces (see MC_record_replay_is_active()) */
XBT_PUBLIC int MC_is_active();

SG_END_DECL

#endif /* SIMGRID_MODELCHECKER_H */
