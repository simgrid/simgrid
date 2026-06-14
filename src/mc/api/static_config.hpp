/* Copyright (c) 2026-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

/* This file contains compile-time options of the smemory module.
 *
 * Since it solves an extremely intensive issue, smemory is carefuly optimized for medium-range applications. If you
 * want to verify an application that is too large to fit the default limits (either too many threads or too long
 * traces), you want to change the limits in this file. */

#ifndef SIMGRID_MC_STATIC_CONFIG_HPP
#define SIMGRID_MC_STATIC_CONFIG_HPP

#include <cstdint>
#include <type_traits>

namespace simgrid::mc {
namespace static_config {
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

/* The maximal amount of threads is arguably very low, but keep in mind that the exploration of large applications will
 * certainly not terminate, while reducing this maximal amount of threads allow for many optimizations. Feel free to
 * increase this value if you want to explore larger apps at a slower pace.*/
constexpr short max_threads = 32;

/* The maximal time_considered can be kept low unless you verify a MPI application using Waitany on a vector of more
 * than 256 requests, or something similar. To be honnest, keeping this value low is not as important as keeping
 * max_threads low. In particular, there is no gain if the max_considered is not the maximal value of a given type such
 * as uint8_t, uint16_t or uint32_t. In other words, the only interesting values are 256, 65535 or 2^32 */
constexpr short max_time_considered = 256;

/* **** End of the configuration part **** */

// Sanity checks
static_assert(max_threads % 8 == 0,
              "smemory::config::max_threads must be a multiple of 8 or the SIMD optimization will break");
static_assert((max_threads & (max_threads - 1)) == 0,
              "max_threads must be a power of 2 (required by Epoch's AID_BITS computation)");
static_assert(page_size != 0 && ((page_size & (page_size - 1)) == 0),
              "smemory::config::page_size must be power of two.");
static_assert(granularity != 0 && ((granularity & (granularity - 1)) == 0),
              "smemory::config::granularity must be power of two.");
static_assert(max_time_considered != 0 && ((max_time_considered & (max_time_considered - 1)) == 0),
              "smemory::config::max_time_considered should be power of two. We are enforcing it for no reason, simply "
              "because doing otherwise brings no good.");
}; // namespace static_config

// Define the mc::mc_aid_t type as the smallest integer that can contain the max_threads value

// Define mc::time_considered_t as the smallest integer that can contain the max_times_considered value
using time_considered_t =
    std::conditional_t<(static_config::max_time_considered < 256), std::uint8_t,
                       std::conditional_t<(static_config::max_time_considered < 65536), std::uint16_t, std::uint32_t>>;

}; // namespace simgrid::mc

#endif