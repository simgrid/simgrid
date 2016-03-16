/* Copyright (c) 2010-2016. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* Copyright (C) 1991, 1992 Free Software Foundation, Inc.
   This file was then part of the GNU C Library. */

#ifndef SIMGRID_MC_MALLOC_HPP
#define SIMGRID_MC_MALLOC_HPP

#include <vector>

#include <xbt/mmalloc.h>

#include "src/mc/mc_forward.hpp"
#include "src/mc/Process.hpp"

namespace simgrid {
namespace mc {

XBT_PRIVATE int mmalloc_compare_heap(Snapshot* snapshot1, Snapshot* snapshot2);
XBT_PRIVATE int mmalloc_linear_compare_heap(xbt_mheap_t heap1, xbt_mheap_t heap2);
XBT_PRIVATE int init_heap_information(xbt_mheap_t heap1, xbt_mheap_t heap2,
                          std::vector<simgrid::mc::IgnoredHeapRegion>* i1,
                          std::vector<simgrid::mc::IgnoredHeapRegion>* i2);
XBT_PRIVATE int compare_heap_area(
  int process_index, const void *area1, const void* area2,
  Snapshot* snapshot1, Snapshot* snapshot2,
  xbt_dynar_t previous, Type* type, int pointer_level);
XBT_PRIVATE void reset_heap_information(void);

}
}

#endif
