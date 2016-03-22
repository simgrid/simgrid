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

#ifdef __cplusplus

SG_END_DECL()

namespace simgrid {
namespace mc {

/**
 *  \brief Find a suitable subrange of candidate duplicates for a given state
 *  \param list dynamic array of states/pairs with candidate duplicates of the current state;
 *  \param ref current state/pair;
 *  \param min (output) index of the beginning of the the subrange
 *  \param max (output) index of the enf of the subrange
 *
 *  Given a suitably ordered array of states/pairs, this function extracts a subrange
 *  (with index *min <= i <= *max) with candidate duplicates of the given state/pair.
 *  This function uses only fast discriminating criterions and does not use the
 *  full state/pair comparison algorithms.
 *
 *  The states/pairs in list MUST be ordered using a (given) weak order
 *  (based on nb_processes and heap_bytes_used).
 *  The subrange is the subrange of "equivalence" of the given state/pair.
 */
// TODO, it would make sense to use std::set instead
// U = some pointer of T (T*, unique_ptr<T>, shared_ptr<T>)
template<class U, class T>
int get_search_interval(
  U* list, std::size_t count, T *ref, int *min, int *max)
{
  int nb_processes = ref->nb_processes;
  int heap_bytes_used = ref->heap_bytes_used;

  int cursor = 0;
  int start = 0;
  int end = count - 1;
  while (start <= end) {
    cursor = (start + end) / 2;
    int nb_processes_test = list[cursor]->nb_processes;
    int heap_bytes_used_test = list[cursor]->heap_bytes_used;

    if (nb_processes_test < nb_processes)
      start = cursor + 1;
    else if (nb_processes_test > nb_processes)
      end = cursor - 1;
    else if (heap_bytes_used_test < heap_bytes_used)
      start = cursor + 1;
    else if (heap_bytes_used_test > heap_bytes_used)
      end = cursor - 1;
    else {
        *min = *max = cursor;
        int previous_cursor = cursor - 1;
        while (previous_cursor >= 0) {
          nb_processes_test = list[previous_cursor]->nb_processes;
          heap_bytes_used_test = list[previous_cursor]->heap_bytes_used;
          if (nb_processes_test != nb_processes || heap_bytes_used_test != heap_bytes_used)
            break;
          *min = previous_cursor;
          previous_cursor--;
        }
        size_t next_cursor = cursor + 1;
        while (next_cursor < count) {
          nb_processes_test = list[next_cursor]->nb_processes;
          heap_bytes_used_test = list[next_cursor]->heap_bytes_used;
          if (nb_processes_test != nb_processes || heap_bytes_used_test != heap_bytes_used)
            break;
          *max = next_cursor;
          next_cursor++;
        }
        return -1;
    }
  }
  return cursor;
}

}
}

#endif

SG_BEGIN_DECL()

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

//#define MC_DEBUG 1
#define MC_VERBOSE 1

/********************************** Miscellaneous **********************************/

XBT_PRIVATE void MC_report_assertion_error(void);
XBT_PRIVATE void MC_report_crash(int status);

SG_END_DECL()

#ifdef __cplusplus

namespace simgrid {
namespace mc {

XBT_PRIVATE void find_object_address(
  std::vector<simgrid::xbt::VmMap> const& maps, simgrid::mc::ObjectInformation* result);

XBT_PRIVATE
int snapshot_compare(int num1, simgrid::mc::Snapshot* s1, int num2, simgrid::mc::Snapshot* s2);

}
}

#endif

#endif
