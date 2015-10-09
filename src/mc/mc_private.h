/* Copyright (c) 2007-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_PRIVATE_H
#define SIMGRID_MC_PRIVATE_H

#include <sys/types.h>

#include "simgrid_config.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#ifndef WIN32
#include <sys/mman.h>
#endif
#include <elfutils/libdw.h>

#include "mc/mc.h"
#include "mc_base.h"
#include "mc/datatypes.h"
#include "xbt/fifo.h"
#include "xbt/config.h"

#include "xbt/function_types.h"
#include "xbt/mmalloc.h"
#include "../simix/smx_private.h"
#include "../xbt/mmalloc/mmprivate.h"
#include "xbt/automaton.h"
#include "xbt/hash.h"
#include <simgrid/msg.h>
#include "xbt/strbuff.h"
#include "xbt/parmap.h"
#include <xbt/base.h>

#include "mc_forward.h"
#include "mc_protocol.h"

SG_BEGIN_DECL()

typedef struct s_mc_function_index_item s_mc_function_index_item_t, *mc_function_index_item_t;

/********************************* MC Global **********************************/

/** Initialisation of the model-checker
 *
 * @param pid     PID of the target process
 * @param socket  FD for the communication socket **in server mode** (or -1 otherwise)
 */
void MC_init_model_checker(pid_t pid, int socket);

XBT_PRIVATE extern FILE *dot_output;
XBT_PRIVATE extern const char* colors[13];
XBT_PRIVATE extern xbt_parmap_t parmap;

XBT_PRIVATE extern int user_max_depth_reached;

XBT_PRIVATE int MC_deadlock_check(void);
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

typedef struct s_mc_comparison_times{
  double nb_processes_comparison_time;
  double bytes_used_comparison_time;
  double stacks_sizes_comparison_time;
  double global_variables_comparison_time;
  double heap_comparison_time;
  double stacks_comparison_time;
}s_mc_comparison_times_t, *mc_comparison_times_t;

extern XBT_PRIVATE __thread mc_comparison_times_t mc_comp_times;
extern XBT_PRIVATE __thread double mc_snapshot_comparison_time;

XBT_PRIVATE int snapshot_compare(void *state1, void *state2);
XBT_PRIVATE void print_comparison_times(void);

//#define MC_DEBUG 1
#define MC_VERBOSE 1

/********************************** Miscellaneous **********************************/

XBT_PRIVATE void MC_dump_stacks(FILE* file);

XBT_PRIVATE void MC_report_assertion_error(void);

XBT_PRIVATE void MC_invalidate_cache(void);

SG_END_DECL()

#endif
