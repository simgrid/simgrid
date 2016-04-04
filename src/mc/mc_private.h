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
#include <xbt/automaton.h>

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

#ifdef __cplusplus
namespace simgrid {
namespace mc {

struct DerefAndCompareByNbProcessesAndUsedHeap {
  template<class X, class Y>
  bool operator()(X const& a, Y const& b)
  {
    return std::make_pair(a->nb_processes, a->heap_bytes_used) <
      std::make_pair(b->nb_processes, b->heap_bytes_used);
  }
};

}
}
#endif

SG_BEGIN_DECL()

/********************************* MC Global **********************************/

XBT_PRIVATE void MC_init_dot_output();

XBT_PRIVATE extern FILE *dot_output;

XBT_PRIVATE extern int user_max_depth_reached;

XBT_PRIVATE void MC_show_deadlock(void);

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

// Move is somewhere else (in the LivenessChecker class, in the Session class?):
extern XBT_PRIVATE xbt_automaton_t property_automaton;

}
}

#endif

#endif
