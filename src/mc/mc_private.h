/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_PRIVATE_H
#define SIMGRID_MC_PRIVATE_H

#include "simgrid_config.h"

#include <sys/types.h>

#include <stdio.h>
#ifndef WIN32
#include <sys/mman.h>
#endif

#include <elfutils/libdw.h>

#include <simgrid/msg.h>
#include <xbt/fifo.h>
#include <xbt/config.h>
#include <xbt/base.h>

#include "mc/mc.h"
#include "mc/datatypes.h"
#include "src/mc/mc_base.h"

#include "src/simix/smx_private.h"
#include "src/xbt/mmalloc/mmprivate.h"

#ifdef __cplusplus
#include "src/mc/mc_forward.hpp"
#include "src/xbt/memory_map.hpp"
#endif

#include "src/mc/mc_protocol.h"

SG_BEGIN_DECL()

/********************************* MC Global **********************************/

XBT_PRIVATE void MC_init_dot_output();

XBT_PRIVATE extern FILE *dot_output;

XBT_PRIVATE extern int user_max_depth_reached;

XBT_PRIVATE void MC_replay(xbt_fifo_t stack);
XBT_PRIVATE void MC_replay_liveness(xbt_fifo_t stack);
XBT_PRIVATE void MC_show_deadlock(smx_simcall_t req);
XBT_PRIVATE void MC_show_stack_safety(xbt_fifo_t stack);
XBT_PRIVATE void MC_dump_stack_safety(xbt_fifo_t stack);
XBT_PRIVATE void MC_show_non_termination(void);

/** Stack (of `mc_state_t`) representing the current position of the
 *  the MC in the exploration graph
 *
 *  It is managed by its head (`xbt_fifo_shift` and `xbt_fifo_unshift`).
 */
XBT_PRIVATE extern xbt_fifo_t mc_stack;

XBT_PRIVATE int get_search_interval(xbt_dynar_t list, void *ref, int *min, int *max);


/****************************** Statistics ************************************/

typedef struct mc_stats {
  unsigned long state_size;
  unsigned long visited_states;
  unsigned long visited_pairs;
  unsigned long expanded_states;
  unsigned long expanded_pairs;
  unsigned long executed_transitions;
} s_mc_stats_t, *mc_stats_t;

XBT_PRIVATE extern mc_stats_t mc_stats;

XBT_PRIVATE void MC_print_statistics(mc_stats_t stats);

/********************************** Snapshot comparison **********************************/

XBT_PRIVATE int snapshot_compare(void *state1, void *state2);

//#define MC_DEBUG 1
#define MC_VERBOSE 1

/********************************** Miscellaneous **********************************/

XBT_PRIVATE void MC_report_assertion_error(void);
XBT_PRIVATE void MC_report_crash(int status);

#ifdef __cplusplus

namespace simgrid {
namespace mc {

XBT_PRIVATE void find_object_address(
  std::vector<simgrid::xbt::VmMap> const& maps, simgrid::mc::ObjectInformation* result);

}
}

#endif

SG_END_DECL()

#endif
