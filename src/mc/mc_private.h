/* Copyright (c) 2007-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef MC_PRIVATE_H
#define MC_PRIVATE_H

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
void MC_init_pid(pid_t pid, int socket);

extern FILE *dot_output;
extern const char* colors[13];
extern xbt_parmap_t parmap;

extern int user_max_depth_reached;

int MC_deadlock_check(void);
void MC_replay(xbt_fifo_t stack);
void MC_replay_liveness(xbt_fifo_t stack);
void MC_show_deadlock(smx_simcall_t req);
void MC_show_stack_safety(xbt_fifo_t stack);
void MC_dump_stack_safety(xbt_fifo_t stack);
void MC_show_non_termination(void);

/** Stack (of `mc_state_t`) representing the current position of the
 *  the MC in the exploration graph
 *
 *  It is managed by its head (`xbt_fifo_shift` and `xbt_fifo_unshift`).
 */
extern xbt_fifo_t mc_stack;

int get_search_interval(xbt_dynar_t list, void *ref, int *min, int *max);


/****************************** Statistics ************************************/

typedef struct mc_stats {
  unsigned long state_size;
  unsigned long visited_states;
  unsigned long visited_pairs;
  unsigned long expanded_states;
  unsigned long expanded_pairs;
  unsigned long executed_transitions;
} s_mc_stats_t, *mc_stats_t;

extern mc_stats_t mc_stats;

void MC_print_statistics(mc_stats_t stats);

/********************************** Snapshot comparison **********************************/

typedef struct s_mc_comparison_times{
  double nb_processes_comparison_time;
  double bytes_used_comparison_time;
  double stacks_sizes_comparison_time;
  double global_variables_comparison_time;
  double heap_comparison_time;
  double stacks_comparison_time;
}s_mc_comparison_times_t, *mc_comparison_times_t;

extern __thread mc_comparison_times_t mc_comp_times;
extern __thread double mc_snapshot_comparison_time;

int snapshot_compare(void *state1, void *state2);
void print_comparison_times(void);

//#define MC_DEBUG 1
#define MC_VERBOSE 1

/********************************** Variables with DWARF **********************************/

void MC_find_object_address(memory_map_t maps, mc_object_info_t result);

/********************************** Miscellaneous **********************************/

typedef struct s_local_variable{
  dw_frame_t subprogram;
  unsigned long ip;
  char *name;
  dw_type_t type;
  void *address;
  int region;
}s_local_variable_t, *local_variable_t;

/* *********** Sets *********** */

typedef struct s_mc_address_set *mc_address_set_t;

mc_address_set_t mc_address_set_new(void);
void mc_address_set_free(mc_address_set_t* p);
void mc_address_add(mc_address_set_t p, const void* value);
bool mc_address_test(mc_address_set_t p, const void* value);

/* *********** Hash *********** */

/** \brief Hash the current state
 *  \param num_state number of states
 *  \param stacks stacks (mc_snapshot_stak_t) used fot the stack unwinding informations
 *  \result resulting hash
 * */
uint64_t mc_hash_processes_state(int num_state, xbt_dynar_t stacks);

/** @brief Dump the stacks of the application processes
 *
 *   This functions is currently not used but it is quite convenient
 *   to call from the debugger.
 *
 *   Does not work when an application thread is running.
 */
void MC_dump_stacks(FILE* file);

void MC_report_assertion_error(void);

SG_END_DECL()

#endif
