/* Copyright (c) 2026-2026. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/3rd-party/catch.hpp"
#include "src/mc/smemory/smemory_config.hpp"

#define private public
#include "VectorClockPool.hpp"

using namespace simgrid::mc::smemory;
using namespace simgrid::mc;

TEST_CASE("VectorClockPool - Initialization and interning rules", "[VectorClockPool]")
{
  VectorClockPool pool;

  SECTION("The index 0 must be for the neutral element")
  {
    REQUIRE(pool.size() == 1);
    const VectorClock& neutral = pool.get(0);

    for (uint32_t i = 0; i < smemory::config::max_threads - 1; ++i) {
      REQUIRE(neutral.clocks[i].get_clock().value() == 0);
      REQUIRE(neutral.clocks[i].get_aid().c_val() == static_cast<int>(i));
    }
  }

  SECTION("The interning must avoid memory dupplication")
  {
    VectorClock vc1;
    vc1.clocks[4]  = Epoch(Aid(4u), Clock(50u));
    vc1.clocks[12] = Epoch(Aid(12u), Clock(120u));

    index_t idx1 = pool.get_or_insert(vc1);
    REQUIRE(idx1 == 1);
    REQUIRE(pool.size() == 2);

    // Insert an object that is strictly the same
    VectorClock vc2;
    vc2.clocks[4]  = Epoch(Aid(4u), Clock(50u));
    vc2.clocks[12] = Epoch(Aid(12u), Clock(120u));

    index_t idx2 = pool.get_or_insert(vc2);
    REQUIRE(idx2 == 1);        // We must have the same index
    REQUIRE(pool.size() == 2); // and the pool must not grow
  }

  SECTION("A small change must create a new entry")
  {
    VectorClock vc1;
    vc1.clocks[12] = Epoch(Aid(12u), Clock(10u));
    index_t idx1   = pool.get_or_insert(vc1);

    VectorClock vc2 = vc1;
    vc2.clocks[12]  = Epoch(Aid(12u), Clock(11u)); // The clock is different
    index_t idx2    = pool.get_or_insert(vc2);

    REQUIRE(idx1 != idx2);
    REQUIRE(pool.size() == 3);
  }
}

TEST_CASE("VectorClockPool - Validate is_past() that uses AVX2", "[VectorClockPool][SIMD]")
{
  VectorClockPool pool;

  VectorClock global_min;
  for (uint32_t i = 0; i < smemory::config::max_threads - 1; ++i) {
    global_min.clocks[i] = Epoch(Aid(i), Clock(100u));
  }

  SECTION("The neutral VectorClock (all clocks to 0) is in the past of global_min")
  {
    REQUIRE(pool.is_past(0, global_min) == true);
  }

  SECTION("An identical VectorClock is considered to be past (equal means past)")
  {
    VectorClock identical = global_min;
    index_t idx           = pool.get_or_insert(identical);
    REQUIRE(pool.is_past(idx, global_min) == true);
  }

  SECTION("A past VectorClock is recognized as such")
  {
    VectorClock smaller = global_min;
    smaller.clocks[0]   = Epoch(Aid(0u), Clock(10u));
    smaller.clocks[2]   = Epoch(Aid(2u), Clock(50u));
    smaller.clocks[3]   = Epoch(Aid(3u), Clock(0u));

    index_t idx = pool.get_or_insert(smaller);
    REQUIRE(pool.is_past(idx, global_min) == true);
  }

  SECTION("Even a single component larger must invalidate the function")
  {
    // boundary limit: index 0
    VectorClock bad_start = global_min;
    bad_start.clocks[0]   = Epoch(Aid(0u), Clock(101u));
    index_t idx_start     = pool.get_or_insert(bad_start);
    REQUIRE(pool.is_past(idx_start, global_min) == false);

    // boundary limit: last used thread
    VectorClock bad_mid                     = global_min;
    bad_mid.clocks[config::max_threads - 2] = Epoch(Aid(config::max_threads - 2), Clock(500u));
    index_t idx_mid                         = pool.get_or_insert(bad_mid);
    REQUIRE(pool.is_past(idx_mid, global_min) == false);

    // boundary limit: last thead position, which is not used in real code (because of INVALID value)
    VectorClock bad_end                     = global_min;
    bad_end.clocks[config::max_threads - 1] = Epoch(Aid(config::max_threads - 2), Clock(500u));
    index_t idx_end                         = pool.get_or_insert(bad_end);
    REQUIRE(pool.is_past(idx_end, global_min) == false);
  }
}

TEST_CASE("VectorClockPool - Garbage Collector (clean_before)", "[VectorClockPool][GC]")
{
  VectorClockPool pool;

  // Create some vector clocks
  VectorClock vc_old1;
  vc_old1.clocks[0] = Epoch(Aid(0u), Clock(10u));
  vc_old1.clocks[1] = Epoch(Aid(1u), Clock(10u));

  VectorClock vc_old2;
  vc_old2.clocks[0] = Epoch(Aid(0u), Clock(20u));
  vc_old2.clocks[1] = Epoch(Aid(1u), Clock(20u));

  VectorClock vc_future;
  vc_future.clocks[0] = Epoch(Aid(0u), Clock(100u));
  vc_future.clocks[1] = Epoch(Aid(1u), Clock(10u));

  // Insert them all in the pool
  index_t idx_old1   = pool.get_or_insert(vc_old1);   // Index 1
  index_t idx_old2   = pool.get_or_insert(vc_old2);   // Index 2
  index_t idx_future = pool.get_or_insert(vc_future); // Index 3

  REQUIRE(idx_old1 == 1); // idx == 0 is for the neutral object
  REQUIRE(idx_old2 == 2);
  REQUIRE(idx_future == 3);
  REQUIRE(pool.size() == 4);

  // Define a global_min obsoleting vc_old1 and vc_old2 but not vc_future
  VectorClock global_min;
  for (uint32_t i = 0; i < simgrid::mc::smemory::config::max_threads - 1; ++i) {
    global_min.clocks[i] = Epoch(Aid(i), Clock(50u));
  }

  SECTION("The neutral element at index 0 must remain, even if it's in the past")
  {
    REQUIRE(pool.is_past(0, global_min) == true);
    pool.clean_before(global_min);

    // The neutral element must remain
    REQUIRE_NOTHROW(pool.get(0));
    REQUIRE(pool.get(0).clocks[0].get_clock().value() == 0);
  }

  SECTION("Cleanup and reuse of free cells in LIFO order")
  {
    pool.clean_before(global_min);

    // The size of the internal container must remain unchanged (no shrink)
    REQUIRE(pool.size() == 4);

    // vc_future must remain (it's not in the past)
    REQUIRE(pool.get(idx_future) == vc_future);

    // Indices 1 and 2 must have been freed. Since we use a stack, 2 will be reused first
    VectorClock new_vc_A;
    new_vc_A.clocks[5] = Epoch(Aid(5u), Clock(999u));

    VectorClock new_vc_B;
    new_vc_B.clocks[6] = Epoch(Aid(6u), Clock(999u));

    index_t new_idx_A = pool.get_or_insert(new_vc_A);
    index_t new_idx_B = pool.get_or_insert(new_vc_B);

    // Free cells must be reused without growing the pool
    REQUIRE(new_idx_A == 2);
    REQUIRE(new_idx_B == 1);
    REQUIRE(pool.size() == 4);
  }

  SECTION("Correct cleanup of the lookup table")
  {
    pool.clean_before(global_min);

    // If we insert an exact copy of vc_old1 (which was cleaned)
    VectorClock vc_old1_copy = vc_old1;
    index_t new_idx          = pool.get_or_insert(vc_old1_copy);

    // it must not be in the previous location. Since 2 was push last, the reinserted element must take this position
    REQUIRE(new_idx != idx_old1);
    REQUIRE(new_idx == 2);
  }
}