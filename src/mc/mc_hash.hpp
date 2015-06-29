/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_HASH_HPP
#define SIMGRID_MC_HASH_HPP

#include <stdio.h>
#include <stdint.h>

#include <vector>

#include "xbt/misc.h"
#include "mc_snapshot.h"

/** \brief Hash the current state
 *  \param num_state number of states
 *  \param stacks stacks (mc_snapshot_stak_t) used fot the stack unwinding informations
 *  \result resulting hash
 * */
XBT_INTERNAL uint64_t mc_hash_processes_state(
  int num_state, std::vector<s_mc_snapshot_stack_t> const& stacks);

/** @brief Dump the stacks of the application processes
 *
 *   This functions is currently not used but it is quite convenient
 *   to call from the debugger.
 *
 *   Does not work when an application thread is running.
 */

#endif
