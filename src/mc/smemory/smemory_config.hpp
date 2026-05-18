/* Copyright (c) 2026-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This file contains compile-time options of the smemory module. Since it solves an extremely intensive issue, this
 * code is   */
/* carefuly optimized for medium-range applications. If you want to verify an application that is too large to fit the
 * default */
/* limits (either too many threads or too long traces), you want to change the limits in this file. */

#ifndef SMEMORY_CONFIG_HPP
#define SMEMORY_CONFIG_HPP

#include <cstdint>

namespace simgrid::mc::smemory::config {
/* Page size in bytes for the two-layer data structures (MemoryAccessTracker and ShadowIntervalMap).
 *
 * The tests assume that the pages are 4kb, but other values could work too, provided that they are powers of 2.
 *
 * Larger pages could reduce the amount of time we search for a given page at the cost of more internal fragmentation in
 * a given page. I also suspect that larger page will hammer the cache lines, but I'm not sure. */
constexpr uintptr_t page_size = 4096;

/* Size in bytes of each memory bucket. The MemoryAccessTracker two bits per bucket of the observed memory to track read
 * and write accesses.
 *
 * The value must be a power of two.
 *
 * Larger values decrease the memory consumption by a linear factor, at the expense of potential false positives in data
 * sharing. */
constexpr short granularity = 1;

// Sanity checks
static_assert(page_size != 0 && ((page_size & (page_size - 1)) == 0),
              "smemory::config::page_size must be power of two.");
static_assert(granularity != 0 && ((granularity & (granularity - 1)) == 0),
              "smemory::config::granularity must be power of two.");
}; // namespace simgrid::mc::smemory::config

#endif